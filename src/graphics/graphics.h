#include "graphics/font.h"
#include "graphics/shader.h"
#include "lib/gpu.h"
#include "lib/maf.h"
#include "util.h"
#include "platform.h"
#include <stdbool.h>
#include <stdint.h>

#pragma once

#define MAX_TRANSFORMS 64
#define MAX_BATCHES 16
#define MAX_DRAWS 256
#define MAX_LOCKS 4

struct Buffer;
struct Canvas;
struct Font;
struct Material;
struct Mesh;
struct Shader;
struct Texture;

typedef void (*StencilCallback)(void* userdata);

typedef enum {
  ARC_MODE_PIE,
  ARC_MODE_OPEN,
  ARC_MODE_CLOSED
} ArcMode;

typedef enum {
  BLEND_ALPHA,
  BLEND_ADD,
  BLEND_SUBTRACT,
  BLEND_MULTIPLY,
  BLEND_LIGHTEN,
  BLEND_DARKEN,
  BLEND_SCREEN,
  BLEND_NONE
} BlendMode;

typedef enum {
  BLEND_ALPHA_MULTIPLY,
  BLEND_PREMULTIPLIED
} BlendAlphaMode;

typedef enum {
  COMPARE_EQUAL,
  COMPARE_NEQUAL,
  COMPARE_LESS,
  COMPARE_LEQUAL,
  COMPARE_GREATER,
  COMPARE_GEQUAL,
  COMPARE_NONE
} CompareMode;

typedef enum {
  STYLE_FILL,
  STYLE_LINE
} DrawStyle;

typedef enum {
  STENCIL_REPLACE,
  STENCIL_INCREMENT,
  STENCIL_DECREMENT,
  STENCIL_INCREMENT_WRAP,
  STENCIL_DECREMENT_WRAP,
  STENCIL_INVERT
} StencilAction;

typedef enum {
  WINDING_CLOCKWISE,
  WINDING_COUNTERCLOCKWISE
} Winding;

typedef struct {
  bool stereo;
  struct Canvas* canvas;
  f32 viewMatrix[2][16];
  f32 projection[2][16];
} Camera;

typedef struct {
  f32 transform[16];
  Color color;
} DrawData;

typedef struct {
  bit alphaSampling : 1;
  bit blendMode : 3; // BlendMode
  bit blendAlphaMode : 1; // BlendAlphaMode
  bit culling : 1;
  bit depthTest : 3; // CompareMode
  bit depthWrite : 1;
  bit lineWidth : 8;
  bit stencilValue: 8;
  bit stencilMode : 3; // CompareMode
  bit winding : 1; // Winding
  bit wireframe : 1;
} Pipeline;

typedef enum {
  STREAM_VERTEX,
  STREAM_INDEX,
  STREAM_DRAW_ID,
  STREAM_TRANSFORM,
  STREAM_COLOR,
  MAX_BUFFER_ROLES
} BufferRole;

typedef enum {
  BATCH_POINTS,
  BATCH_LINES,
  BATCH_TRIANGLES,
  BATCH_PLANE,
  BATCH_BOX,
  BATCH_ARC,
  BATCH_SPHERE,
  BATCH_CYLINDER,
  BATCH_SKYBOX,
  BATCH_TEXT,
  BATCH_FILL,
  BATCH_MESH
} BatchType;

typedef union {
  struct { DrawStyle style; } triangles;
  struct { DrawStyle style; } plane;
  struct { DrawStyle style; } box;
  struct { DrawStyle style; ArcMode mode; f32 r1; f32 r2; u32 segments; } arc;
  struct { f32 r1; f32 r2; bool capped; u32 segments; } cylinder;
  struct { u32 segments; } sphere;
  struct { f32 u; f32 v; f32 w; f32 h; } fill;
  struct { struct Mesh* object; DrawMode mode; u32 rangeStart; u32 rangeCount; u32 instances; f32* pose; } mesh;
} BatchParams;

typedef struct {
  BatchType type;
  BatchParams params;
  DrawMode drawMode;
  DefaultShader shader;
  Pipeline* pipeline;
  struct Material* material;
  struct Texture* diffuseTexture;
  struct Texture* environmentMap;
  mat4 transform;
  u32 vertexCount;
  u32 indexCount;
  f32** vertices;
  u16** indices;
  u16* baseVertex;
  bool instanced;
} BatchRequest;

typedef struct {
  BatchType type;
  BatchParams params;
  DrawMode drawMode;
  struct Canvas* canvas;
  struct Shader* shader;
  Pipeline pipeline;
  struct Material* material;
  mat4 transforms;
  Color* colors;
  struct { u32 start; u32 count; } cursors[MAX_BUFFER_ROLES];
  u32 count;
  bool instanced;
} Batch;

// Base
bool lovrGraphicsInit(bool gammaCorrect);
void lovrGraphicsDestroy(void);
void lovrGraphicsPresent(void);
void lovrGraphicsCreateWindow(WindowFlags* flags);
u32 lovrGraphicsGetWidth(void);
u32 lovrGraphicsGetHeight(void);
f32 lovrGraphicsGetPixelDensity(void);
void lovrGraphicsSetCamera(Camera* camera, bool clear);
struct Buffer* lovrGraphicsGetIdentityBuffer(void);
#define lovrGraphicsGetFeatures lovrGpuGetFeatures
#define lovrGraphicsGetLimits lovrGpuGetLimits
#define lovrGraphicsGetStats lovrGpuGetStats

// State
void lovrGraphicsReset(void);
bool lovrGraphicsGetAlphaSampling(void);
void lovrGraphicsSetAlphaSampling(bool sample);
Color lovrGraphicsGetBackgroundColor(void);
void lovrGraphicsSetBackgroundColor(Color color);
void lovrGraphicsGetBlendMode(BlendMode* mode, BlendAlphaMode* alphaMode);
void lovrGraphicsSetBlendMode(BlendMode mode, BlendAlphaMode alphaMode);
struct Canvas* lovrGraphicsGetCanvas(void);
void lovrGraphicsSetCanvas(struct Canvas* canvas);
Color lovrGraphicsGetColor(void);
void lovrGraphicsSetColor(Color color);
bool lovrGraphicsIsCullingEnabled(void);
void lovrGraphicsSetCullingEnabled(bool culling);
TextureFilter lovrGraphicsGetDefaultFilter(void);
void lovrGraphicsSetDefaultFilter(TextureFilter filter);
void lovrGraphicsGetDepthTest(CompareMode* mode, bool* write);
void lovrGraphicsSetDepthTest(CompareMode depthTest, bool write);
struct Font* lovrGraphicsGetFont(void);
void lovrGraphicsSetFont(struct Font* font);
bool lovrGraphicsIsGammaCorrect(void);
u8 lovrGraphicsGetLineWidth(void);
void lovrGraphicsSetLineWidth(u8 width);
f32 lovrGraphicsGetPointSize(void);
void lovrGraphicsSetPointSize(f32 size);
struct Shader* lovrGraphicsGetShader(void);
void lovrGraphicsSetShader(struct Shader* shader);
void lovrGraphicsGetStencilTest(CompareMode* mode, u8* value);
void lovrGraphicsSetStencilTest(CompareMode mode, u8 value);
Winding lovrGraphicsGetWinding(void);
void lovrGraphicsSetWinding(Winding winding);
bool lovrGraphicsIsWireframe(void);
void lovrGraphicsSetWireframe(bool wireframe);

// Transforms
void lovrGraphicsPush(void);
void lovrGraphicsPop(void);
void lovrGraphicsOrigin(void);
void lovrGraphicsTranslate(vec3 translation);
void lovrGraphicsRotate(quat rotation);
void lovrGraphicsScale(vec3 scale);
void lovrGraphicsMatrixTransform(mat4 transform);
void lovrGraphicsSetProjection(mat4 projection);

// Rendering
void lovrGraphicsClear(Color* color, f32* depth, u8* stencil);
void lovrGraphicsDiscard(bool color, bool depth, bool stencil);
void lovrGraphicsBatch(BatchRequest* req);
void lovrGraphicsFlush(void);
void lovrGraphicsFlushCanvas(struct Canvas* canvas);
void lovrGraphicsFlushShader(struct Shader* shader);
void lovrGraphicsFlushMaterial(struct Material* material);
void lovrGraphicsFlushMesh(struct Mesh* mesh);
void lovrGraphicsPoints(u32 count, f32** vertices);
void lovrGraphicsLine(u32 count, f32** vertices);
void lovrGraphicsTriangle(DrawStyle style, struct Material* material, u32 count, f32** vertices);
void lovrGraphicsPlane(DrawStyle style, struct Material* material, mat4 transform, f32 u, f32 v, f32 w, f32 h);
void lovrGraphicsBox(DrawStyle style, struct Material* material, mat4 transform);
void lovrGraphicsArc(DrawStyle style, ArcMode mode, struct Material* material, mat4 transform, f32 r1, f32 r2, u32 segments);
void lovrGraphicsCircle(DrawStyle style, struct Material* material, mat4 transform, u32 segments);
void lovrGraphicsCylinder(struct Material* material, mat4 transform, f32 r1, f32 r2, bool capped, u32 segments);
void lovrGraphicsSphere(struct Material* material, mat4 transform, u32 segments);
void lovrGraphicsSkybox(struct Texture* texture, f32 angle, f32 ax, f32 ay, f32 az);
void lovrGraphicsPrint(const char* str, usize length, mat4 transform, f32 wrap, HorizontalAlign halign, VerticalAlign valign);
void lovrGraphicsFill(struct Texture* texture, f32 u, f32 v, f32 w, f32 h);
#define lovrGraphicsStencil lovrGpuStencil
#define lovrGraphicsCompute lovrGpuCompute

// GPU

typedef struct {
  bool compute;
  bool singlepass;
} GpuFeatures;

typedef struct {
  bool initialized;
  f32 pointSizes[2];
  u32 textureSize;
  u32 textureMSAA;
  f32 textureAnisotropy;
  usize blockSize;
  usize blockAlign;
} GpuLimits;

typedef struct {
  u32 shaderSwitches;
  u32 drawCalls;
} GpuStats;

typedef struct {
  struct Mesh* mesh;
  struct Canvas* canvas;
  struct Shader* shader;
  Pipeline pipeline;
  DrawMode drawMode;
  u32 instances;
  u32 rangeStart;
  u32 rangeCount;
  bit width : 15;
  bit height : 15;
  bit stereo : 1;
} DrawCommand;

void lovrGpuInit(bool srgb, getProcAddressProc getProcAddress);
void lovrGpuDestroy(void);
void lovrGpuClear(struct Canvas* canvas, Color* color, f32* depth, u8* stencil);
void lovrGpuCompute(struct Shader* shader, u32 x, u32 y, u32 z);
void lovrGpuDiscard(struct Canvas* canvas, bool color, bool depth, bool stencil);
void lovrGpuDraw(DrawCommand* draw);
void lovrGpuStencil(StencilAction action, u8 replaceValue, StencilCallback callback, void* userdata);
void lovrGpuPresent(void);
void lovrGpuDirtyTexture(void);
void* lovrGpuLock(void);
void lovrGpuUnlock(void* lock);
void lovrGpuDestroyLock(void* lock);
const GpuFeatures* lovrGpuGetFeatures(void);
const GpuLimits* lovrGpuGetLimits(void);
const GpuStats* lovrGpuGetStats(void);
