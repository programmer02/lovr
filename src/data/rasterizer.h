#include "util.h"
#include "types.h"
#include "lib/stb/stb_truetype.h"

#pragma once

#define GLYPH_PADDING 1

struct Blob;
struct TextureData;

typedef struct Rasterizer {
  Ref ref;
  stbtt_fontinfo font;
  struct Blob* blob;
  f32 size;
  f32 scale;
  u32 glyphCount;
  i32 height;
  i32 advance;
  i32 ascent;
  i32 descent;
} Rasterizer;

typedef struct {
  i32 x;
  i32 y;
  u32 w;
  u32 h;
  u32 tw;
  u32 th;
  i32 dx;
  i32 dy;
  i32 advance;
  struct TextureData* data;
} Glyph;

Rasterizer* lovrRasterizerInit(Rasterizer* rasterizer, struct Blob* blob, f32 size);
#define lovrRasterizerCreate(...) lovrRasterizerInit(lovrAlloc(Rasterizer), __VA_ARGS__)
void lovrRasterizerDestroy(void* ref);
bool lovrRasterizerHasGlyph(Rasterizer* fontData, u32 character);
bool lovrRasterizerHasGlyphs(Rasterizer* fontData, const char* str);
void lovrRasterizerLoadGlyph(Rasterizer* fontData, u32 character, Glyph* glyph);
i32 lovrRasterizerGetKerning(Rasterizer* fontData, u32 left, u32 right);
