#include "util.h"
#include "data/textureData.h"
#include "graphics/opengl.h"
#include "data/modelData.h"
#include "types.h"

#pragma once

struct TextureData;

typedef enum {
  TEXTURE_2D,
  TEXTURE_CUBE,
  TEXTURE_ARRAY,
  TEXTURE_VOLUME
} TextureType;

typedef struct Texture {
  Ref ref;
  TextureType type;
  TextureFormat format;
  u32 width;
  u32 height;
  u32 depth;
  u32 mipmapCount;
  TextureFilter filter;
  TextureWrap wrap;
  u32 msaa;
  bool srgb;
  bool mipmaps;
  bool allocated;
  GPU_TEXTURE_FIELDS
} Texture;

Texture* lovrTextureInit(Texture* texture, TextureType type, struct TextureData** slices, u32 sliceCount, bool srgb, bool mipmaps, u32 msaa);
Texture* lovrTextureInitFromHandle(Texture* texture, u32 handle, TextureType type);
#define lovrTextureCreate(...) lovrTextureInit(lovrAlloc(Texture), __VA_ARGS__)
#define lovrTextureCreateFromHandle(...) lovrTextureInitFromHandle(lovrAlloc(Texture), __VA_ARGS__)
void lovrTextureDestroy(void* ref);
void lovrTextureAllocate(Texture* texture, u32 width, u32 height, u32 depth, TextureFormat format);
void lovrTextureReplacePixels(Texture* texture, struct TextureData* data, u32 x, u32 y, u32 slice, u32 mipmap);
u32 lovrTextureGetWidth(Texture* texture, u32 mipmap);
u32 lovrTextureGetHeight(Texture* texture, u32 mipmap);
u32 lovrTextureGetDepth(Texture* texture, u32 mipmap);
u32 lovrTextureGetMipmapCount(Texture* texture);
u32 lovrTextureGetMSAA(Texture* texture);
TextureType lovrTextureGetType(Texture* texture);
TextureFormat lovrTextureGetFormat(Texture* texture);
TextureFilter lovrTextureGetFilter(Texture* texture);
void lovrTextureSetFilter(Texture* texture, TextureFilter filter);
TextureWrap lovrTextureGetWrap(Texture* texture);
void lovrTextureSetWrap(Texture* texture, TextureWrap wrap);
