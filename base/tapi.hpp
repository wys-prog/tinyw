#pragma once

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

typedef uint8_t*(*Tfunc_MemoryGetPointer)();
typedef uint64_t(*Tfunc_MemoryGetSize)();

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

#define err(x) therr(func, x)