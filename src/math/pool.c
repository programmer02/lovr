#include "math/pool.h"
#include "lib/err.h"
#include <stdlib.h>

static const usize sizeOfMathType[] = {
  [MATH_VEC3] = 4 * sizeof(f32),
  [MATH_QUAT] = 4 * sizeof(f32),
  [MATH_MAT4] = 16 * sizeof(f32)
};

Pool* lovrPoolInit(Pool* pool, usize size) {
  pool->size = size;
  pool->data = malloc(pool->size + POOL_ALIGN - 1);
  pool->head = (u8*) ALIGN((u8*) pool->data + POOL_ALIGN - 1, POOL_ALIGN);
  lovrAssert(pool->data, "Out of memory");
  return pool;
}

void lovrPoolDestroy(void* ref) {
  Pool* pool = ref;
  free(pool->data);
}

f32* lovrPoolAllocate(Pool* pool, MathType type) {
  usize size = sizeOfMathType[type];
  lovrAssert(pool->usage + size <= pool->size, "Pool overflow");
  f32* p = (f32*) (pool->head + pool->usage);
  pool->usage += size;
  return p;
}

void lovrPoolDrain(Pool* pool) {
  pool->usage = 0;
}

usize lovrPoolGetSize(Pool* pool) {
  return pool->size;
}

usize lovrPoolGetUsage(Pool* pool) {
  return pool->usage;
}

f32* lovrPoolAllocateVec3(Pool* pool) { return lovrPoolAllocate(pool, MATH_VEC3); }
f32* lovrPoolAllocateQuat(Pool* pool) { return lovrPoolAllocate(pool, MATH_QUAT); }
f32* lovrPoolAllocateMat4(Pool* pool) { return lovrPoolAllocate(pool, MATH_MAT4); }
