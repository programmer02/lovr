#include "data/textureData.h"
#include "filesystem/file.h"
#include "lib/stb/stb_image.h"
#include "lib/stb/stb_image_write.h"
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define FOUR_CC(a, b, c, d) ((u32) (((d)<<24) | ((c)<<16) | ((b)<<8) | (a)))

static usize getPixelSize(TextureFormat format) {
  switch (format) {
    case FORMAT_RGB: return 3;
    case FORMAT_RGBA: return 4;
    case FORMAT_RGBA4: return 2;
    case FORMAT_RGBA16F: return 8;
    case FORMAT_RGBA32F: return 16;
    case FORMAT_R16F: return 2;
    case FORMAT_R32F: return 4;
    case FORMAT_RG16F: return 4;
    case FORMAT_RG32F: return 8;
    case FORMAT_RGB5A1: return 2;
    case FORMAT_RGB10A2: return 4;
    case FORMAT_RG11B10F: return 4;
    case FORMAT_D16: return 2;
    case FORMAT_D32F: return 4;
    case FORMAT_D24S8: return 4;
    default: return 0;
  }
}

// Modified from ddsparse (https://bitbucket.org/slime73/ddsparse)
static bool parseDDS(u8* data, usize size, TextureData* textureData) {
  if (size < sizeof(u32) + sizeof(DDSHeader) || *(u32*) data != FOUR_CC('D', 'D', 'S', ' ')) {
    return false;
  }

  // Header
  usize offset = sizeof(u32);
  DDSHeader* header = (DDSHeader*) (data + offset);
  offset += sizeof(DDSHeader);
  if (header->size != sizeof(DDSHeader) || header->format.size != sizeof(DDSPixelFormat)) {
    return false;
  }

  // DX10 header
  if ((header->format.flags & DDPF_FOURCC) && (header->format.fourCC == FOUR_CC('D', 'X', '1', '0'))) {
    if (size < (sizeof(u32) + sizeof(DDSHeader) + sizeof(DDSHeader10))) {
      return false;
    }

    DDSHeader10* header10 = (DDSHeader10*) (data + offset);
    offset += sizeof(DDSHeader10);

    // Only accept 2D textures
    D3D10ResourceDimension dimension = header10->resourceDimension;
    if (dimension != D3D10_RESOURCE_DIMENSION_TEXTURE2D && dimension != D3D10_RESOURCE_DIMENSION_UNKNOWN) {
      return false;
    }

    // Can't deal with texture arrays and cubemaps.
    if (header10->arraySize > 1) {
      return false;
    }

    // Ensure DXT 1/3/5
    switch (header10->dxgiFormat) {
      case DXGI_FORMAT_BC1_TYPELESS:
      case DXGI_FORMAT_BC1_UNORM:
      case DXGI_FORMAT_BC1_UNORM_SRGB:
        textureData->format = FORMAT_DXT1;
        break;
      case DXGI_FORMAT_BC2_TYPELESS:
      case DXGI_FORMAT_BC2_UNORM:
      case DXGI_FORMAT_BC2_UNORM_SRGB:
        textureData->format = FORMAT_DXT3;
        break;
      case DXGI_FORMAT_BC3_TYPELESS:
      case DXGI_FORMAT_BC3_UNORM:
      case DXGI_FORMAT_BC3_UNORM_SRGB:
        textureData->format = FORMAT_DXT5;
        break;
      default:
        return false;
    }
  } else {
    if ((header->format.flags & DDPF_FOURCC) == 0) {
      return false;
    }

    // Ensure DXT 1/3/5
    switch (header->format.fourCC) {
      case FOUR_CC('D', 'X', 'T', '1'): textureData->format = FORMAT_DXT1; break;
      case FOUR_CC('D', 'X', 'T', '3'): textureData->format = FORMAT_DXT3; break;
      case FOUR_CC('D', 'X', 'T', '5'): textureData->format = FORMAT_DXT5; break;
      default: return false;
    }
  }

  u32 width = textureData->width = header->width;
  u32 height = textureData->height = header->height;
  u32 mipmapCount = header->mipMapCount;
  usize blockBytes = 0;

  switch (textureData->format) {
    case FORMAT_DXT1: blockBytes = 8; break;
    case FORMAT_DXT3: blockBytes = 16; break;
    case FORMAT_DXT5: blockBytes = 16; break;
    default: break;
  }

  // Load mipmaps
  for (u32 i = 0; i < mipmapCount; i++) {
    u32 numBlocksWide = width ? MAX(1, (width + 3) / 4) : 0;
    u32 numBlocksHigh = height ? MAX(1, (height + 3) / 4) : 0;
    usize mipmapSize = numBlocksWide * numBlocksHigh * blockBytes;

    // Overflow check
    if (mipmapSize == 0 || (offset + mipmapSize) > size) {
      vec_deinit(&textureData->mipmaps);
      return false;
    }

    Mipmap mipmap = { .width = width, .height = height, .size = mipmapSize, .data = &data[offset] };
    vec_push(&textureData->mipmaps, mipmap);
    offset += mipmapSize;
    width = MAX(width >> 1, 1);
    height = MAX(height >> 1, 1);
  }

  textureData->blob.data = NULL;

  return true;
}

TextureData* lovrTextureDataInit(TextureData* textureData, u32 width, u32 height, u8 value, TextureFormat format) {
  lovrAssert(width > 0 && height > 0, "TextureData dimensions must be positive");
  lovrAssert(format != FORMAT_DXT1 && format != FORMAT_DXT3 && format != FORMAT_DXT5, "Blank TextureData cannot be compressed");
  usize pixelSize = getPixelSize(format);
  usize size = width * height * pixelSize;
  textureData->width = width;
  textureData->height = height;
  textureData->format = format;
  textureData->blob.size = size;
  textureData->blob.data = malloc(size);
  lovrAssert(textureData->blob.data, "Out of memory");
  memset(textureData->blob.data, value, size);
  vec_init(&textureData->mipmaps);
  return textureData;
}

TextureData* lovrTextureDataInitFromBlob(TextureData* textureData, Blob* blob, bool flip) {
  vec_init(&textureData->mipmaps);

  if (parseDDS(blob->data, blob->size, textureData)) {
    textureData->source = blob;
    lovrRetain(blob);
    return textureData;
  }

  i32 width, height;
  i32 size = (i32) blob->size;
  stbi_set_flip_vertically_on_load(flip);
  if (stbi_is_hdr_from_memory(blob->data, size)) {
    textureData->format = FORMAT_RGBA32F;
    textureData->blob.data = stbi_loadf_from_memory(blob->data, size, &width, &height, NULL, 4);
  } else {
    textureData->format = FORMAT_RGBA;
    textureData->blob.data = stbi_load_from_memory(blob->data, size, &width, &height, NULL, 4);
  }

  if (!textureData->blob.data || width <= 0 || height <= 0) {
    lovrThrow("Could not load texture data from '%s'", blob->name);
    free(textureData);
    return NULL;
  }

  textureData->width = (u32) width;
  textureData->height = (u32) height;

  return textureData;
}

Color lovrTextureDataGetPixel(TextureData* textureData, u32 x, u32 y) {
  lovrAssert(textureData->blob.data, "TextureData does not have any pixel data");
  lovrAssert(x < textureData->width && y < textureData->height, "getPixel coordinates must be within TextureData bounds");
  usize index = (textureData->height - (y + 1)) * textureData->width + x;
  usize pixelSize = getPixelSize(textureData->format);
  union { u8* u; f32* f; } pixel = { (u8*) textureData->blob.data + pixelSize * index };
  switch (textureData->format) {
    case FORMAT_RGB: return (Color) { pixel.u[0] / 255.f, pixel.u[1] / 255.f, pixel.u[2] / 255.f, 1.f };
    case FORMAT_RGBA: return (Color) { pixel.u[0] / 255.f, pixel.u[1] / 255.f, pixel.u[2] / 255.f, pixel.u[3] / 255.f };
    case FORMAT_RGBA32F: return (Color) { pixel.f[0], pixel.f[1], pixel.f[2], pixel.f[3] };
    case FORMAT_R32F: return (Color) { pixel.f[0], 1.f, 1.f, 1.f };
    case FORMAT_RG32F: return (Color) { pixel.f[0], pixel.f[1], 1.f, 1.f };
    default: lovrThrow("Unsupported format for TextureData:getPixel");
  }
}

void lovrTextureDataSetPixel(TextureData* textureData, u32 x, u32 y, Color color) {
  lovrAssert(textureData->blob.data, "TextureData does not have any pixel data");
  lovrAssert(x < textureData->width && y < textureData->height, "setPixel coordinates must be within TextureData bounds");
  usize index = (textureData->height - (y + 1)) * textureData->width + x;
  usize pixelSize = getPixelSize(textureData->format);
  union { u8* u; f32* f; } pixel = { (u8*) textureData->blob.data + pixelSize * index };
  switch (textureData->format) {
    case FORMAT_RGB:
      pixel.u[0] = (u8) (color.r * 255.f + .5f);
      pixel.u[1] = (u8) (color.g * 255.f + .5f);
      pixel.u[2] = (u8) (color.b * 255.f + .5f);
      break;

    case FORMAT_RGBA:
      pixel.u[0] = (u8) (color.r * 255.f + .5f);
      pixel.u[1] = (u8) (color.g * 255.f + .5f);
      pixel.u[2] = (u8) (color.b * 255.f + .5f);
      pixel.u[3] = (u8) (color.a * 255.f + .5f);
      break;

    case FORMAT_RGBA32F:
      pixel.f[0] = color.r;
      pixel.f[1] = color.g;
      pixel.f[2] = color.b;
      pixel.f[3] = color.a;
      break;

    case FORMAT_R32F:
      pixel.f[0] = color.r;
      break;

    case FORMAT_RG32F:
      pixel.f[0] = color.r;
      pixel.f[1] = color.g;
      break;

    default: lovrThrow("Unsupported format for TextureData:setPixel");
  }
}

static void writeCallback(void* context, void* data, i32 size) {
  if (size > 0) {
    File* file = context;
    lovrFileWrite(file, data, (usize) size);
  }
}

bool lovrTextureDataEncode(TextureData* textureData, const char* filename) {
  File file;
  lovrFileInit(memset(&file, 0, sizeof(File)), filename);
  if (lovrFileOpen(&file, OPEN_WRITE)) {
    return false;
  }
  lovrAssert(textureData->format == FORMAT_RGB || textureData->format == FORMAT_RGBA, "Only RGB and RGBA TextureData can be encoded");
  i32 width = (i32) textureData->width;
  i32 height = (i32) textureData->height;
  i32 components = textureData->format == FORMAT_RGB ? 3 : 4;
  void* data = (u8*) textureData->blob.data + (textureData->height - 1) * textureData->width * (u32) components;
  i32 stride = -width * components;
  bool success = stbi_write_png_to_func(writeCallback, &file, width, height, components, data, stride);
  lovrFileDestroy(&file);
  return success;
}

void lovrTextureDataDestroy(void* ref) {
  TextureData* textureData = ref;
  lovrRelease(Blob, textureData->source);
  vec_deinit(&textureData->mipmaps);
  lovrBlobDestroy(ref);
}
