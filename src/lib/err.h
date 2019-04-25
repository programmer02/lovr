#include <stdarg.h>

#pragma once

#ifdef _WIN32
#define ERR_NORETURN __declspec(noreturn)
#else
#define ERR_NORETURN __attribute__((noreturn))
#endif

#ifdef _WIN32
#define ERR_THREAD  __declspec(thread)
#else
#define ERR_THREAD __thread
#endif

typedef void (*err_fn)(void* userdata, const char* format, va_list args);
extern ERR_THREAD err_fn err_handler;
extern ERR_THREAD void* err_context;

void err_setHandler(err_fn handler, void* context);
void ERR_NORETURN lovrThrow(const char* format, ...);
#define lovrAssert(c, ...) ((void) (c || (lovrThrow(__VA_ARGS__), 0)))
