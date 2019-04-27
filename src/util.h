#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#pragma once

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef size_t usize;
typedef float f32;
typedef double f64;
typedef unsigned bit;

#define LOVR_VERSION_MAJOR 0
#define LOVR_VERSION_MINOR 12
#define LOVR_VERSION_PATCH 0
#define LOVR_VERSION_ALIAS "Mushroom Detector"

#define PI 3.14159265358979323846f
#define MAX(a, b) (a > b ? a : b)
#define MIN(a, b) (a < b ? a : b)
#define CLAMP(x, min, max) MAX(min, MIN(max, x))
#define ALIGN(p, n) ((uintptr_t) (p) & -n)
#define PRINT_SIZEOF(T) i32(*_o)[sizeof(T)]=1

#ifdef _WIN32
#define LOVR_EXPORT __declspec(dllexport)
#define LOVR_THREAD_LOCAL __declspec(thread)
#else
#define LOVR_EXPORT __attribute__((visibility("default")))
#define LOVR_THREAD_LOCAL __thread
#endif
