#pragma once

#include <cstdint>
#include <cstddef>
#include <string>

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
#endif