#include "graphics/font.h"
#include "graphics/texture.h"
#include "data/textureData.h"
#include "lib/utf.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static f32* lovrFontAlignLine(f32* x, f32* lineEnd, f32 width, HorizontalAlign halign) {
  while (x < lineEnd) {
    if (halign == ALIGN_CENTER) {
      *x -= width / 2.f;
    } else if (halign == ALIGN_RIGHT) {
      *x -= width;
    }

    x += 8;
  }

  return x;
}

Font* lovrFontInit(Font* font, Rasterizer* rasterizer) {
  lovrRetain(rasterizer);
  font->rasterizer = rasterizer;
  font->lineHeight = 1.f;
  font->pixelDensity = (f32) font->rasterizer->height;
  map_init(&font->kerning);

  // Atlas
  u32 padding = 1;
  font->atlas.x = padding;
  font->atlas.y = padding;
  font->atlas.width = 128;
  font->atlas.height = 128;
  font->atlas.padding = padding;
  map_init(&font->atlas.glyphs);

  // Set initial atlas size
  while (font->atlas.height < 4 * rasterizer->size) {
    lovrFontExpandTexture(font);
  }

  // Create the texture
  lovrFontCreateTexture(font);

  return font;
}

void lovrFontDestroy(void* ref) {
  Font* font = ref;
  lovrRelease(Rasterizer, font->rasterizer);
  lovrRelease(Texture, font->texture);
  const char* key;
  map_iter_t iter = map_iter(&font->atlas.glyphs);
  while ((key = map_next(&font->atlas.glyphs, &iter)) != NULL) {
    Glyph* glyph = map_get(&font->atlas.glyphs, key);
    lovrRelease(TextureData, glyph->data);
  }
  map_deinit(&font->atlas.glyphs);
  map_deinit(&font->kerning);
}

Rasterizer* lovrFontGetRasterizer(Font* font) {
  return font->rasterizer;
}

void lovrFontRender(Font* font, const char* str, usize length, f32 wrap, HorizontalAlign halign, f32* vertices, u16* indices, u16 baseVertex) {
  FontAtlas* atlas = &font->atlas;
  bool flip = font->flip;

  f32 cx = 0.f;
  f32 cy = -font->rasterizer->height * .8f * (flip ? -1.f : 1.f);
  f32 u = (f32) atlas->width;
  f32 v = (f32) atlas->height;
  f32 scale = 1.f / font->pixelDensity;

  const char* start = str;
  const char* end = str + length;
  u32 previous = '\0';
  u32 codepoint;
  usize bytes;

  f32* vertexCursor = vertices;
  u16* indexCursor = indices;
  f32* lineStart = vertices;
  u16 I = baseVertex;

  while ((bytes = utf8_decode(str, end, &codepoint)) > 0) {

    // Newlines
    if (codepoint == '\n' || (wrap && cx * scale > wrap && codepoint == ' ')) {
      lineStart = lovrFontAlignLine(lineStart, vertexCursor, cx, halign);
      cx = 0.f;
      cy -= font->rasterizer->height * font->lineHeight * (flip ? -1.f : 1.f);
      previous = '\0';
      str += bytes;
      continue;
    }

    // Tabs
    if (codepoint == '\t') {
      Glyph* space = lovrFontGetGlyph(font, ' ');
      cx += space->advance * 4.f;
      str += bytes;
      continue;
    }

    // Kerning
    cx += lovrFontGetKerning(font, previous, codepoint);
    previous = codepoint;

    // Get glyph
    Glyph* glyph = lovrFontGetGlyph(font, codepoint);

    // Start over if texture was repacked
    if (u != atlas->width || v != atlas->height) {
      lovrFontRender(font, start, length, wrap, halign, vertices, indices, baseVertex);
      return;
    }

    // Triangles
    if (glyph->w > 0 && glyph->h > 0) {
      f32 x1 = cx + glyph->dx - GLYPH_PADDING;
      f32 y1 = cy + (glyph->dy + GLYPH_PADDING) * (flip ? -1.f : 1.f);
      f32 x2 = x1 + glyph->tw;
      f32 y2 = y1 - glyph->th * (flip ? -1.f : 1.f);
      f32 s1 = glyph->x / u;
      f32 t1 = (glyph->y + glyph->th) / v;
      f32 s2 = (glyph->x + glyph->tw) / u;
      f32 t2 = glyph->y / v;

      memcpy(vertexCursor, (f32[32]) {
        x1, y1, 0.f, 0.f, 0.f, 0.f, s1, t1,
        x1, y2, 0.f, 0.f, 0.f, 0.f, s1, t2,
        x2, y1, 0.f, 0.f, 0.f, 0.f, s2, t1,
        x2, y2, 0.f, 0.f, 0.f, 0.f, s2, t2
      }, 32 * sizeof(f32));

      memcpy(indexCursor, (u16[6]) { I + 0, I + 1, I + 2, I + 2, I + 1, I + 3 }, 6 * sizeof(u16));

      vertexCursor += 32;
      indexCursor += 6;
      I += 4;
    }

    // Advance cursor
    cx += glyph->advance;
    str += bytes;
  }

  // Align the last line
  lovrFontAlignLine(lineStart, vertexCursor, cx, halign);
}

void lovrFontMeasure(Font* font, const char* str, usize length, f32 wrap, f32* width, u32* lineCount, u32* glyphCount) {
  f32 x = 0.f;
  const char* end = str + length;
  usize bytes;
  u32 previous = '\0';
  u32 codepoint;
  f32 scale = 1.f / font->pixelDensity;
  *width = 0.f;
  *lineCount = 0;
  *glyphCount = 0;

  while ((bytes = utf8_decode(str, end, &codepoint)) > 0) {
    if (codepoint == '\n' || (wrap && x * scale > wrap && codepoint == ' ')) {
      *width = MAX(*width, x * scale);
      (*lineCount)++;
      x = 0.f;
      previous = '\0';
      str += bytes;
      continue;
    }

    // Tabs
    if (codepoint == '\t') {
      Glyph* space = lovrFontGetGlyph(font, ' ');
      x += space->advance * 4.f;
      str += bytes;
      continue;
    }

    Glyph* glyph = lovrFontGetGlyph(font, codepoint);

    if (glyph->w > 0 && glyph->h > 0) {
      (*glyphCount)++;
    }

    x += glyph->advance + lovrFontGetKerning(font, previous, codepoint);
    previous = codepoint;
    str += bytes;
  }

  *width = MAX(*width, x * scale);
}

f32 lovrFontGetHeight(Font* font) {
  return font->rasterizer->height / font->pixelDensity;
}

f32 lovrFontGetAscent(Font* font) {
  return font->rasterizer->ascent / font->pixelDensity;
}

f32 lovrFontGetDescent(Font* font) {
  return font->rasterizer->descent / font->pixelDensity;
}

f32 lovrFontGetBaseline(Font* font) {
  return font->rasterizer->height * .8f / font->pixelDensity;
}

f32 lovrFontGetLineHeight(Font* font) {
  return font->lineHeight;
}

void lovrFontSetLineHeight(Font* font, f32 lineHeight) {
  font->lineHeight = lineHeight;
}

bool lovrFontIsFlipEnabled(Font* font) {
  return font->flip;
}

void lovrFontSetFlipEnabled(Font* font, bool flip) {
  font->flip = flip;
}

i32 lovrFontGetKerning(Font* font, u32 left, u32 right) {
  char key[12];
  snprintf(key, 12, "%d,%d", left, right);

  i32* entry = map_get(&font->kerning, key);
  if (entry) {
    return *entry;
  }

  i32 kerning = lovrRasterizerGetKerning(font->rasterizer, left, right);
  map_set(&font->kerning, key, kerning);
  return kerning;
}

f32 lovrFontGetPixelDensity(Font* font) {
  return font->pixelDensity;
}

void lovrFontSetPixelDensity(Font* font, f32 pixelDensity) {
  if (pixelDensity <= 0.f) {
    pixelDensity = font->rasterizer->height;
  }

  font->pixelDensity = pixelDensity;
}

Glyph* lovrFontGetGlyph(Font* font, u32 codepoint) {
  char key[6];
  snprintf(key, 6, "%d", codepoint);

  FontAtlas* atlas = &font->atlas;
  map_glyph_t* glyphs = &atlas->glyphs;
  Glyph* glyph = map_get(glyphs, key);

  // Add the glyph to the atlas if it isn't there
  if (!glyph) {
    Glyph g;
    lovrRasterizerLoadGlyph(font->rasterizer, codepoint, &g);
    map_set(glyphs, key, g);
    glyph = map_get(glyphs, key);
    lovrFontAddGlyph(font, glyph);
  }

  return glyph;
}

void lovrFontAddGlyph(Font* font, Glyph* glyph) {
  FontAtlas* atlas = &font->atlas;

  // Don't waste space on empty glyphs
  if (glyph->w == 0 && glyph->h == 0) {
    return;
  }

  // If the glyph does not fit, you must acquit (new row)
  if (atlas->x + glyph->tw > atlas->width - 2 * atlas->padding) {
    atlas->x = atlas->padding;
    atlas->y += atlas->rowHeight + atlas->padding;
    atlas->rowHeight = 0;
  }

  // Expand the texture if needed. Expanding the texture re-adds all the glyphs, so we can return.
  if (atlas->y + glyph->th > atlas->height - 2 * atlas->padding) {
    lovrFontExpandTexture(font);
    return;
  }

  // Keep track of glyph's position in the atlas
  glyph->x = atlas->x;
  glyph->y = atlas->y;

  // Paste glyph into texture
  lovrTextureReplacePixels(font->texture, glyph->data, atlas->x, atlas->y, 0, 0);

  // Advance atlas cursor
  atlas->x += glyph->tw + atlas->padding;
  atlas->rowHeight = MAX(atlas->rowHeight, glyph->th);
}

void lovrFontExpandTexture(Font* font) {
  FontAtlas* atlas = &font->atlas;

  if (atlas->width == atlas->height) {
    atlas->width *= 2;
  } else {
    atlas->height *= 2;
  }

  if (!font->texture) {
    return;
  }

  // Recreate the texture
  lovrFontCreateTexture(font);

  // Reset the cursor
  atlas->x = atlas->padding;
  atlas->y = atlas->padding;
  atlas->rowHeight = 0;

  // Re-pack all the glyphs
  const char* key;
  map_iter_t iter = map_iter(&atlas->glyphs);
  while ((key = map_next(&atlas->glyphs, &iter)) != NULL) {
    Glyph* glyph = map_get(&atlas->glyphs, key);
    lovrFontAddGlyph(font, glyph);
  }
}

// TODO we only need the TextureData here to clear the texture, but it's a big waste of memory.
// Could look into using glClearTexImage when supported to make this more efficient.
void lovrFontCreateTexture(Font* font) {
  lovrRelease(Texture, font->texture);
  TextureData* textureData = lovrTextureDataCreate(font->atlas.width, font->atlas.height, 0x0, FORMAT_RGB);
  font->texture = lovrTextureCreate(TEXTURE_2D, &textureData, 1, false, false, 0);
  lovrTextureSetFilter(font->texture, (TextureFilter) { .mode = FILTER_BILINEAR });
  lovrTextureSetWrap(font->texture, (TextureWrap) { .s = WRAP_CLAMP, .t = WRAP_CLAMP });
  lovrRelease(TextureData, textureData);
}
