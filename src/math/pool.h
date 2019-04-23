#include "util.h"
#include "types.h"

#define POOL_ALIGN 16
#define DEFAULT_POOL_SIZE (640 * 1024)

#pragma once

typedef enum {
  MATH_VEC3,
  MATH_QUAT,
  MATH_MAT4,
  MAX_MATH_TYPES
} MathType;

typedef struct {
  Ref ref;
  f32* data;
  usize size;
  usize usage;
  u8* head;
} Pool;

Pool* lovrPoolInit(Pool* pool, usize size);
#define lovrPoolCreate(...) lovrPoolInit(lovrAlloc(Pool), __VA_ARGS__)
void lovrPoolDestroy(void* ref);
f32* lovrPoolAllocate(Pool* pool, MathType type);
void lovrPoolDrain(Pool* pool);
usize lovrPoolGetSize(Pool* pool);
usize lovrPoolGetUsage(Pool* pool);

// For you, LuaJIT
LOVR_EXPORT f32* lovrPoolAllocateVec3(Pool* pool);
LOVR_EXPORT f32* lovrPoolAllocateQuat(Pool* pool);
LOVR_EXPORT f32* lovrPoolAllocateMat4(Pool* pool);
