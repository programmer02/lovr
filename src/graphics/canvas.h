#include "util.h"
#include "lib/gpu.h"

#pragma once

#define MAX_CANVAS_ATTACHMENTS 4

struct Texture;
struct TextureData;

typedef struct Canvas Canvas;
extern const usize sizeof_Canvas;
Canvas* lovrCanvasInit(Canvas* canvas, u32 width, u32 height, CanvasFlags flags);
Canvas* lovrCanvasInitFromHandle(Canvas* canvas, u32 width, u32 height, CanvasFlags flags, u32 framebuffer, u32 depthBuffer, u32 resolveBuffer, u32 attachmentCount, bool immortal);
#define lovrCanvasCreate(...) lovrCanvasInit(lovrAlloc(Canvas), __VA_ARGS__)
#define lovrCanvasCreateFromHandle(...) lovrCanvasInitFromHandle(lovrAlloc(Canvas), __VA_ARGS__)
void lovrCanvasDestroy(void* ref);
const Attachment* lovrCanvasGetAttachments(Canvas* canvas, u32* count);
void lovrCanvasSetAttachments(Canvas* canvas, Attachment* attachments, u32 count);
void lovrCanvasResolve(Canvas* canvas);
bool lovrCanvasIsStereo(Canvas* canvas);
u32 lovrCanvasGetWidth(Canvas* canvas);
u32 lovrCanvasGetHeight(Canvas* canvas);
u32 lovrCanvasGetMSAA(Canvas* canvas);
struct Texture* lovrCanvasGetDepthTexture(Canvas* canvas);
struct TextureData* lovrCanvasNewTextureData(Canvas* canvas, u32 index);
