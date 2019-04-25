#include "util.h"
#include "data/rasterizer.h"
#include "types.h"
#include "lib/map/map.h"

#pragma once

struct Rasterizer;
struct Texture;

typedef map_t(Glyph) map_glyph_t;

typedef enum {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
} HorizontalAlign;

typedef enum {
  ALIGN_TOP,
  ALIGN_MIDDLE,
  ALIGN_BOTTOM
} VerticalAlign;

typedef struct {
  u32 x;
  u32 y;
  u32 width;
  u32 height;
  u32 rowHeight;
  u32 padding;
  map_glyph_t glyphs;
} FontAtlas;

typedef struct Font {
  Ref ref;
  struct Rasterizer* rasterizer;
  struct Texture* texture;
  FontAtlas atlas;
  map_int_t kerning;
  f32 lineHeight;
  f32 pixelDensity;
  bool flip;
} Font;

Font* lovrFontInit(Font* font, struct Rasterizer* rasterizer);
#define lovrFontCreate(...) lovrFontInit(lovrAlloc(Font), __VA_ARGS__)
void lovrFontDestroy(void* ref);
struct Rasterizer* lovrFontGetRasterizer(Font* font);
void lovrFontRender(Font* font, const char* str, usize length, f32 wrap, HorizontalAlign halign, f32* vertices, u16* indices, u16 baseVertex);
void lovrFontMeasure(Font* font, const char* string, usize length, f32 wrap, f32* width, u32* lineCount, u32* glyphCount);
f32 lovrFontGetHeight(Font* font);
f32 lovrFontGetAscent(Font* font);
f32 lovrFontGetDescent(Font* font);
f32 lovrFontGetBaseline(Font* font);
f32 lovrFontGetLineHeight(Font* font);
void lovrFontSetLineHeight(Font* font, f32 lineHeight);
bool lovrFontIsFlipEnabled(Font* font);
void lovrFontSetFlipEnabled(Font* font, bool flip);
i32 lovrFontGetKerning(Font* font, u32 a, u32 b);
f32 lovrFontGetPixelDensity(Font* font);
void lovrFontSetPixelDensity(Font* font, f32 pixelDensity);
Glyph* lovrFontGetGlyph(Font* font, u32 codepoint);
void lovrFontAddGlyph(Font* font, Glyph* glyph);
void lovrFontExpandTexture(Font* font);
void lovrFontCreateTexture(Font* font);
