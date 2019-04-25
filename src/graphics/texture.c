#include "graphics/texture.h"

u32 lovrTextureGetWidth(Texture* texture, u32 mipmap) {
  return MAX(texture->width >> mipmap, 1);
}

u32 lovrTextureGetHeight(Texture* texture, u32 mipmap) {
  return MAX(texture->height >> mipmap, 1);
}

u32 lovrTextureGetDepth(Texture* texture, u32 mipmap) {
  return texture->type == TEXTURE_VOLUME ? MAX(texture->depth >> mipmap, 1) : texture->depth;
}

u32 lovrTextureGetMipmapCount(Texture* texture) {
  return texture->mipmapCount;
}

TextureType lovrTextureGetType(Texture* texture) {
  return texture->type;
}

TextureFormat lovrTextureGetFormat(Texture* texture) {
  return texture->format;
}

TextureFilter lovrTextureGetFilter(Texture* texture) {
  return texture->filter;
}

TextureWrap lovrTextureGetWrap(Texture* texture) {
  return texture->wrap;
}
