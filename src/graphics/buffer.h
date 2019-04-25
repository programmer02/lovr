#include "util.h"
#include "types.h"
#include "graphics/opengl.h"

#pragma once

typedef enum {
  BUFFER_VERTEX,
  BUFFER_INDEX,
  BUFFER_UNIFORM,
  BUFFER_SHADER_STORAGE,
  BUFFER_GENERIC,
  MAX_BUFFER_TYPES
} BufferType;

typedef enum {
  USAGE_STATIC,
  USAGE_DYNAMIC,
  USAGE_STREAM
} BufferUsage;

typedef struct Buffer {
  Ref ref;
  void* data;
  usize size;
  usize flushFrom;
  usize flushTo;
  bool readable;
  BufferType type;
  BufferUsage usage;
  GPU_BUFFER_FIELDS
} Buffer;

Buffer* lovrBufferInit(Buffer* buffer, usize size, void* data, BufferType type, BufferUsage usage, bool readable);
#define lovrBufferCreate(...) lovrBufferInit(lovrAlloc(Buffer), __VA_ARGS__)
void lovrBufferDestroy(void* ref);
usize lovrBufferGetSize(Buffer* buffer);
bool lovrBufferIsReadable(Buffer* buffer);
BufferUsage lovrBufferGetUsage(Buffer* buffer);
void* lovrBufferMap(Buffer* buffer, usize offset);
void lovrBufferFlushRange(Buffer* buffer, usize offset, usize size);
void lovrBufferMarkRange(Buffer* buffer, usize offset, usize size);
void lovrBufferFlush(Buffer* buffer);
