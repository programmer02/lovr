#include "graphics/canvas.h"
#include "graphics/graphics.h"

const Attachment* lovrCanvasGetAttachments(Canvas* canvas, u32* count) {
  if (count) *count = canvas->attachmentCount;
  return canvas->attachments;
}

void lovrCanvasSetAttachments(Canvas* canvas, Attachment* attachments, u32 count) {
  lovrAssert(count > 0, "A Canvas must have at least one attached Texture");
  lovrAssert(count <= MAX_CANVAS_ATTACHMENTS, "Only %d textures can be attached to a Canvas, got %d\n", MAX_CANVAS_ATTACHMENTS, count);

  if (!canvas->needsAttach && count == canvas->attachmentCount && !memcmp(canvas->attachments, attachments, count * sizeof(Attachment))) {
    return;
  }

  lovrGraphicsFlushCanvas(canvas);

  for (u32 i = 0; i < count; i++) {
    Texture* texture = attachments[i].texture;
    u32 slice = attachments[i].slice;
    u32 level = attachments[i].level;
    u32 width = lovrTextureGetWidth(texture, level);
    u32 height = lovrTextureGetHeight(texture, level);
    u32 depth = lovrTextureGetDepth(texture, level);
    u32 mipmaps = lovrTextureGetMipmapCount(texture);
    bool hasDepthBuffer = canvas->flags.depth.enabled;
    lovrAssert(slice < depth, "Invalid attachment slice (Texture has %d, got %d)", depth, slice + 1);
    lovrAssert(level < mipmaps, "Invalid attachment mipmap level (Texture has %d, got %d)", mipmaps, level + 1);
    lovrAssert(!hasDepthBuffer || width == canvas->width, "Texture width of %d does not match Canvas width (%d)", width, canvas->width);
    lovrAssert(!hasDepthBuffer || height == canvas->height, "Texture height of %d does not match Canvas height (%d)", height, canvas->height);
    lovrAssert(texture->msaa == canvas->flags.msaa, "Texture MSAA does not match Canvas MSAA");
    lovrRetain(texture);
  }

  for (u32 i = 0; i < canvas->attachmentCount; i++) {
    lovrRelease(Texture, canvas->attachments[i].texture);
  }

  memcpy(canvas->attachments, attachments, count * sizeof(Attachment));
  canvas->attachmentCount = count;
  canvas->needsAttach = true;
}

bool lovrCanvasIsStereo(Canvas* canvas) {
  return canvas->flags.stereo;
}

u32 lovrCanvasGetWidth(Canvas* canvas) {
  return canvas->width;
}

u32 lovrCanvasGetHeight(Canvas* canvas) {
  return canvas->height;
}

u32 lovrCanvasGetMSAA(Canvas* canvas) {
  return canvas->flags.msaa;
}

Texture* lovrCanvasGetDepthTexture(Canvas* canvas) {
  return canvas->depth.texture;
}
