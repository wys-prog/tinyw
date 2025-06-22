#pragma once

#include <string>
#include <vector>

#include "glob.hpp"
#include "tasks.hpp"

TinyWDeclStart

inline const uint8_t *BytesOf_File_tinyc_h = (uint8_t*)R"(#ifndef __LIB_TINYC_H__
#define __LIB_TINYC_H__

#include <stdint.h>
#include <stddef.h>

#define nameof(x) #x
#define typeof(x) typeid(x)
#if defined(__GNUC__) || defined(__clang__)
#define func __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define func __FUNCSIG__
#else
#define func __func__
#endif
#ifdef _WIN32
#define cstrdup _strdup
#else
#define cstrdup strdup
#endif 

typedef uint8_t*(*Tfunc_MemoryGetPointer)();
typedef uint64_t(*Tfunc_MemoryGetSize)();

#define err(x) therr(func, x)

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
#define TINYW_EXPORT extern "C" __declspec(dllexport)
#else
#define TINYW_EXPORT extern "C" __attribute__((visibility("default")))
#endif

#define TINYW_CPU_MODULE(START_FN, INIT_FN, STOP_FN) \
    TINYW_EXPORT void start() { START_FN(); } \
    TINYW_EXPORT void stop()  { STOP_FN();  } \
    TINYW_EXPORT void init( \
        uint8_t* (*get_pointer)(), \
        uint64_t (*get_size)(), \
        uint64_t argc, char *const argv[] \
    ) { INIT_FN(get_pointer, get_size, argc, argv); }

#define TINYW_GPU_MODULE(START_FN, STOP_FN, SEND_BYTES_FN, INIT_FN) \
    TINYW_EXPORT void start() { START_FN(); } \
    TINYW_EXPORT void stop()  { STOP_FN();  } \
    TINYW_EXPORT void send_bytes(uint8_t* bytes, uint64_t len) { SEND_BYTES_FN(bytes, len); } \
    TINYW_EXPORT void init(uint64_t argc, char *const argv[]) { INIT_FN(argc, argv); }

#define TINYW_MEMORY_MODULE(GET_POINTER_FN, GET_SIZE_FN, CLEAR_FN, INIT_FN) \
    TINYW_EXPORT uint8_t* get_pointer() { return GET_POINTER_FN(); } \
    TINYW_EXPORT uint64_t get_size()    { return GET_SIZE_FN();    } \
    TINYW_EXPORT void clear()           { CLEAR_FN();              } \
    TINYW_EXPORT void init(uint64_t argc, char *const argv[]) { INIT_FN(argc, argv); }

#ifdef __cplusplus
}

#define try_x(x) do { \
    try { \
        x; \
    } catch (const std::runtime_error &e) { \
        therr(func, std::string("C++ Exception caught: ") + e.what()); \
    } catch (const std::exception &e) { \
        therr(func, std::string("C++ Exception caught: ") + e.what()); \
    } catch (...) { \
        therr(func, "Unknown C++ Exception caught"); \
    } \
} while(0)

#include <string>
#include <sstream>
#include <stdexcept>

inline void therr(const std::string &from, const std::string &what) {
  std::stringstream ss;
  ss << "\n> [i:err]: " << from << ":\n";
  std::istringstream iss(what);
  std::string line;
  while (std::getline(iss, line)) {
    ss << "> " << line << std::endl;
  }

  throw std::runtime_error(ss.str());
}


#endif // C++
#endif // def?)";

inline GenericTask copy_built_in_files(const fs::path &prefix) {
  return GenericTask("copying built-in files", [&](auto report_progress){
    std::ofstream file(prefix / "tinyc.h");
    for (size_t i = 0; i < sizeof(BytesOf_File_tinyc_h); i++) {
      file.put(BytesOf_File_tinyc_h[i]);
      report_progress((float)i / sizeof(BytesOf_File_tinyc_h));
    }
    file.close();
  });

}

TinyWDeclEnd