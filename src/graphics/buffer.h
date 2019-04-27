#include "util.h"
#include "lib/gpu.h"

#pragma once

typedef struct Buffer Buffer;
extern const usize sizeof_Buffer;
Buffer* lovrBufferInit(Buffer* buffer, usize size, void* data, BufferType type, BufferUsage usage, bool readable);
void lovrBufferDestroy(void* ref);
usize lovrBufferGetSize(Buffer* buffer);
bool lovrBufferIsReadable(Buffer* buffer);
BufferUsage lovrBufferGetUsage(Buffer* buffer);
void* lovrBufferMap(Buffer* buffer, usize offset);
void lovrBufferFlushRange(Buffer* buffer, usize offset, usize size);
void lovrBufferMarkRange(Buffer* buffer, usize offset, usize size);
void lovrBufferFlush(Buffer* buffer);
