#include "util.h"
#include "types.h"

#pragma once

typedef struct Blob {
  Ref ref;
  void* data;
  usize size;
  const char* name;
} Blob;

Blob* lovrBlobInit(Blob* blob, void* data, usize size, const char* name);
#define lovrBlobCreate(...) lovrBlobInit(lovrAlloc(Blob), __VA_ARGS__)
void lovrBlobDestroy(void* ref);
