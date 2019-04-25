#pragma clang diagnostic ignored "-Wconversion"
#include "data/rasterizer.h"
#include "data/blob.h"
#include "data/textureData.h"
#include "resources/VarelaRound.ttf.h"
#include "lib/utf.h"
#include "lib/stb/stb_truetype.h"
#include "msdfgen-c.h"
#include <stdlib.h>
#include <math.h>

Rasterizer* lovrRasterizerInit(Rasterizer* rasterizer, Blob* blob, f32 size) {
  stbtt_fontinfo* font = &rasterizer->font;
  u8* data = blob ? blob->data : VarelaRound_ttf;
  if (!stbtt_InitFont(font, data, stbtt_GetFontOffsetForIndex(data, 0))) {
    lovrThrow("Problem loading font");
  }

  lovrRetain(blob);
  rasterizer->blob = blob;
  rasterizer->size = size;
  rasterizer->scale = stbtt_ScaleForMappingEmToPixels(font, size);
  rasterizer->glyphCount = (u32) font->numGlyphs;

  i32 ascent, descent, linegap;
  stbtt_GetFontVMetrics(font, &ascent, &descent, &linegap);
  rasterizer->ascent = roundf(ascent * rasterizer->scale);
  rasterizer->descent = roundf(descent * rasterizer->scale);
  rasterizer->height = roundf((ascent - descent + linegap) * rasterizer->scale);

  i32 x0, y0, x1, y1;
  stbtt_GetFontBoundingBox(font, &x0, &y0, &x1, &y1);
  rasterizer->advance = roundf(x1 * rasterizer->scale);

  return rasterizer;
}

void lovrRasterizerDestroy(void* ref) {
  Rasterizer* rasterizer = ref;
  lovrRelease(Blob, rasterizer->blob);
}

bool lovrRasterizerHasGlyph(Rasterizer* rasterizer, u32 character) {
  return stbtt_FindGlyphIndex(&rasterizer->font, (i32) character) != 0;
}

bool lovrRasterizerHasGlyphs(Rasterizer* rasterizer, const char* str) {
  const char* end = str + strlen(str);
  u32 codepoint;
  usize bytes;

  bool hasGlyphs = true;
  while ((bytes = utf8_decode(str, end, &codepoint)) > 0) {
    hasGlyphs &= lovrRasterizerHasGlyph(rasterizer, codepoint);
    str += bytes;
  }
  return hasGlyphs;
}

void lovrRasterizerLoadGlyph(Rasterizer* rasterizer, u32 character, Glyph* glyph) {
  i32 glyphIndex = stbtt_FindGlyphIndex(&rasterizer->font, (i32) character);
  lovrAssert(glyphIndex, "No font glyph found for character code %d, try using Rasterizer:hasGlyphs", character);

  // Trace glyph outline
  stbtt_vertex* vertices;
  i32 vertexCount = stbtt_GetGlyphShape(&rasterizer->font, glyphIndex, &vertices);
  msShape* shape = msShapeCreate();
  msContour* contour = NULL;
  f32 x = 0.f;
  f32 y = 0.f;

  for (i32 i = 0; i < vertexCount; i++) {
    stbtt_vertex vertex = vertices[i];
    f32 x2 = vertex.x * rasterizer->scale;
    f32 y2 = vertex.y * rasterizer->scale;

    switch (vertex.type) {
      case STBTT_vmove:
        contour = msShapeAddContour(shape);
        break;

      case STBTT_vline:
        msContourAddLinearEdge(contour, x, y, x2, y2);
        break;

      case STBTT_vcurve: {
        f32 cx = vertex.cx * rasterizer->scale;
        f32 cy = vertex.cy * rasterizer->scale;
        msContourAddQuadraticEdge(contour, x, y, cx, cy, x2, y2);
        break;
      }

      case STBTT_vcubic: {
        f32 cx1 = vertex.cx * rasterizer->scale;
        f32 cy1 = vertex.cy * rasterizer->scale;
        f32 cx2 = vertex.cx1 * rasterizer->scale;
        f32 cy2 = vertex.cy1 * rasterizer->scale;
        msContourAddCubicEdge(contour, x, y, cx1, cy1, cx2, cy2, x2, y2);
        break;
      }
    }

    x = x2;
    y = y2;
  }

  i32 advance, bearing;
  stbtt_GetGlyphHMetrics(&rasterizer->font, glyphIndex, &advance, &bearing);

  i32 x0, y0, x1, y1;
  stbtt_GetGlyphBox(&rasterizer->font, glyphIndex, &x0, &y0, &x1, &y1);

  bool empty = stbtt_IsGlyphEmpty(&rasterizer->font, glyphIndex);

  // Initialize glyph data
  glyph->x = 0;
  glyph->y = 0;
  glyph->w = empty ? 0 : ceilf((x1 - x0) * rasterizer->scale);
  glyph->h = empty ? 0 : ceilf((y1 - y0) * rasterizer->scale);
  glyph->tw = glyph->w + 2 * GLYPH_PADDING;
  glyph->th = glyph->h + 2 * GLYPH_PADDING;
  glyph->dx = empty ? 0 : roundf(bearing * rasterizer->scale);
  glyph->dy = empty ? 0 : roundf(y1 * rasterizer->scale);
  glyph->advance = roundf(advance * rasterizer->scale);
  glyph->data = lovrTextureDataCreate(glyph->tw, glyph->th, 0, FORMAT_RGB);

  // Render SDF
  f32 tx = GLYPH_PADDING + -glyph->dx;
  f32 ty = GLYPH_PADDING + glyph->h - glyph->dy;
  msShapeNormalize(shape);
  msEdgeColoringSimple(shape, 3., 0);
  msGenerateMSDF(glyph->data->blob.data, glyph->tw, glyph->th, shape, 4.f, 1.f, 1.f, tx, ty);
  msShapeDestroy(shape);
}

i32 lovrRasterizerGetKerning(Rasterizer* rasterizer, u32 left, u32 right) {
  return stbtt_GetCodepointKernAdvance(&rasterizer->font, left, right) * rasterizer->scale;
}
