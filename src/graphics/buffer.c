#include "graphics/buffer.h"

usize lovrBufferGetSize(Buffer* buffer) {
  return buffer->size;
}

bool lovrBufferIsReadable(Buffer* buffer) {
  return buffer->readable;
}

BufferUsage lovrBufferGetUsage(Buffer* buffer) {
  return buffer->usage;
}

void lovrBufferMarkRange(Buffer* buffer, usize offset, usize size) {
  usize end = offset + size;
  buffer->flushFrom = MIN(buffer->flushFrom, offset);
  buffer->flushTo = MAX(buffer->flushTo, end);
}

void lovrBufferFlush(Buffer* buffer) {
  if (buffer->flushTo <= buffer->flushFrom) {
    return;
  }

  lovrBufferFlushRange(buffer, buffer->flushFrom, buffer->flushTo - buffer->flushFrom);
  buffer->flushFrom = SIZE_MAX;
  buffer->flushTo = 0;
}
