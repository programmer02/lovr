#include <stdint.h>

#pragma once

struct Texture;

typedef struct Color { float r, g, b, a; } Color;

typedef enum {
  DRAW_POINTS,
  DRAW_LINES,
  DRAW_LINE_LOOP,
  DRAW_LINE_STRIP,
  DRAW_TRIANGLES,
  DRAW_TRIANGLE_STRIP,
  DRAW_TRIANGLE_FAN
} DrawMode;

typedef enum {
  TEXTURE_2D,
  TEXTURE_CUBE,
  TEXTURE_ARRAY,
  TEXTURE_VOLUME
} TextureType;

typedef enum {
  FORMAT_RGB,
  FORMAT_RGBA,
  FORMAT_RGBA4,
  FORMAT_RGBA16F,
  FORMAT_RGBA32F,
  FORMAT_R16F,
  FORMAT_R32F,
  FORMAT_RG16F,
  FORMAT_RG32F,
  FORMAT_RGB5A1,
  FORMAT_RGB10A2,
  FORMAT_RG11B10F,
  FORMAT_D16,
  FORMAT_D32F,
  FORMAT_D24S8,
  FORMAT_DXT1,
  FORMAT_DXT3,
  FORMAT_DXT5
} TextureFormat;

typedef enum {
  FILTER_NEAREST,
  FILTER_BILINEAR,
  FILTER_TRILINEAR,
  FILTER_ANISOTROPIC
} FilterMode;

typedef struct {
  FilterMode mode;
  float anisotropy;
} TextureFilter;

typedef enum {
  WRAP_CLAMP,
  WRAP_REPEAT,
  WRAP_MIRRORED_REPEAT
} WrapMode;

typedef struct {
  WrapMode s;
  WrapMode t;
  WrapMode r;
} TextureWrap;

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

typedef struct {
  struct Texture* texture;
  uint32_t slice;
  uint32_t level;
} Attachment;

typedef struct {
  struct {
    bool enabled;
    bool readable;
    TextureFormat format;
  } depth;
  bool stereo;
  uint32_t msaa;
  bool mipmaps;
} CanvasFlags;
