#include "graphics/graphics.h"
#include "graphics/buffer.h"
#include "graphics/canvas.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "graphics/texture.h"
#include "data/rasterizer.h"
#include "event/event.h"
#include "math/math.h"
#include "lib/maf.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static struct {
  bool initialized;
  bool gammaCorrect;
  u32 width;
  u32 height;
  Camera camera;
  struct Shader* defaultShaders[MAX_DEFAULT_SHADERS];
  struct Material* defaultMaterial;
  struct Font* defaultFont;
  TextureFilter defaultFilter;
  f32 transforms[MAX_TRANSFORMS][16];
  u32 transform;
  Color backgroundColor;
  struct Canvas* canvas;
  Color color;
  struct Font* font;
  Pipeline pipeline;
  f32 pointSize;
  struct Shader* shader;
  struct Mesh* mesh;
  struct Mesh* instancedMesh;
  struct Buffer* identityBuffer;
  struct Buffer* buffers[MAX_BUFFER_ROLES];
  u32 cursors[MAX_BUFFER_ROLES];
  void* locks[MAX_BUFFER_ROLES][MAX_LOCKS];
  Batch batches[MAX_BATCHES];
  u8 batchCount;
} state;

static void gammaCorrectColor(Color* color) {
  if (state.gammaCorrect) {
    color->r = lovrMathGammaToLinear(color->r);
    color->g = lovrMathGammaToLinear(color->g);
    color->b = lovrMathGammaToLinear(color->b);
  }
}

static void onCloseWindow(void) {
  lovrEventPush((Event) { .type = EVENT_QUIT, .data.quit = { false, 0 } });
}

static void onResizeWindow(u32 width, u32 height) {
  state.width = width;
  state.height = height;
}

static const u32 BUFFER_COUNTS[] = {
  [STREAM_VERTEX] = (1 << 16) - 1,
  [STREAM_INDEX] = 1 << 16,
  [STREAM_DRAW_ID] = (1 << 16) - 1,
#ifdef LOVR_WEBGL // Temporarily work around bug where big UBOs don't work
  [STREAM_TRANSFORM] = MAX_DRAWS,
  [STREAM_COLOR] = MAX_DRAWS
#else
  [STREAM_TRANSFORM] = MAX_DRAWS * MAX_BATCHES * 2,
  [STREAM_COLOR] = MAX_DRAWS * MAX_BATCHES * 2
#endif
};

static const usize BUFFER_STRIDES[] = {
  [STREAM_VERTEX] = 8 * sizeof(f32),
  [STREAM_INDEX] = sizeof(u16),
  [STREAM_DRAW_ID] = sizeof(u8),
  [STREAM_TRANSFORM] = 16 * sizeof(f32),
  [STREAM_COLOR] = 4 * sizeof(f32)
};

static const BufferType BUFFER_TYPES[] = {
  [STREAM_VERTEX] = BUFFER_VERTEX,
  [STREAM_INDEX] = BUFFER_INDEX,
  [STREAM_DRAW_ID] = BUFFER_GENERIC,
  [STREAM_TRANSFORM] = BUFFER_UNIFORM,
  [STREAM_COLOR] = BUFFER_UNIFORM
};

static void lovrGraphicsInitBuffers() {
  for (u32 i = 0; i < MAX_BUFFER_ROLES; i++) {
    state.buffers[i] = lovrBufferCreate(BUFFER_COUNTS[i] * BUFFER_STRIDES[i], NULL, BUFFER_TYPES[i], USAGE_STREAM, false);
  }

  // The identity buffer is used for autoinstanced meshes and instanced primitives and maps the
  // instance ID to a vertex attribute.  Its contents never change, so they are initialized here.
  state.identityBuffer = lovrBufferCreate(MAX_DRAWS, NULL, BUFFER_VERTEX, USAGE_STATIC, false);
  u8* id = lovrBufferMap(state.identityBuffer, 0);
  for (usize i = 0; i < MAX_DRAWS; i++) id[i] = i;
  lovrBufferFlushRange(state.identityBuffer, 0, MAX_DRAWS);

  Buffer* vertexBuffer = state.buffers[STREAM_VERTEX];
  usize stride = BUFFER_STRIDES[STREAM_VERTEX];

  MeshAttribute position = { .buffer = vertexBuffer, .offset = 0, .stride = stride, .type = F32, .components = 3 };
  MeshAttribute normal = { .buffer = vertexBuffer, .offset = 12, .stride = stride, .type = F32, .components = 3 };
  MeshAttribute texCoord = { .buffer = vertexBuffer, .offset = 24, .stride = stride, .type = F32, .components = 2 };
  MeshAttribute drawId = { .buffer = state.buffers[STREAM_DRAW_ID], .type = U8, .components = 1, .integer = true };
  MeshAttribute identity = { .buffer = state.identityBuffer, .type = U8, .components = 1, .divisor = 1, .integer = true };

  state.mesh = lovrMeshCreate(DRAW_TRIANGLES, NULL, 0);
  lovrMeshAttachAttribute(state.mesh, "lovrPosition", &position);
  lovrMeshAttachAttribute(state.mesh, "lovrNormal", &normal);
  lovrMeshAttachAttribute(state.mesh, "lovrTexCoord", &texCoord);
  lovrMeshAttachAttribute(state.mesh, "lovrDrawID", &drawId);

  state.instancedMesh = lovrMeshCreate(DRAW_TRIANGLES, NULL, 0);
  lovrMeshAttachAttribute(state.instancedMesh, "lovrPosition", &position);
  lovrMeshAttachAttribute(state.instancedMesh, "lovrNormal", &normal);
  lovrMeshAttachAttribute(state.instancedMesh, "lovrTexCoord", &texCoord);
  lovrMeshAttachAttribute(state.instancedMesh, "lovrDrawID", &identity);
}

static void* lovrGraphicsMapBuffer(BufferRole role, u32 count) {
  Buffer* buffer = state.buffers[role];
  u32 limit = BUFFER_COUNTS[role];
  lovrAssert(count <= limit, "Whoa there!  Tried to get %d elements from a buffer that only has %d elements.", count, limit);

  if (state.cursors[role] + count > limit) {
    lovrGraphicsFlush();
    state.cursors[role] = 0;

    // Locks are placed as late as possible, causing the last lock to never get placed.  Whenever we
    // wrap around a buffer, we gotta place that last missing lock.
    state.locks[role][MAX_LOCKS - 1] = lovrGpuLock();
  }

  // Wait on any pending locks for the mapped region(s)
  u32 firstLock = state.cursors[role] / (BUFFER_COUNTS[role] / MAX_LOCKS);
  u32 lastLock = MIN(state.cursors[role] + count, BUFFER_COUNTS[role] - 1) / (BUFFER_COUNTS[role] / MAX_LOCKS);
  for (u32 i = firstLock; i <= lastLock; i++) {
    if (state.locks[role][i]) {
      lovrGpuUnlock(state.locks[role][i]);
      lovrGpuDestroyLock(state.locks[role][i]);
      state.locks[role][i] = NULL;
    }
  }

  return lovrBufferMap(buffer, state.cursors[role] * BUFFER_STRIDES[role]);
}

static bool areBatchParamsEqual(BatchType typeA, BatchType typeB, BatchParams* a, BatchParams* b) {
  if (typeA != typeB) return false;

  switch (typeA) {
    case BATCH_TRIANGLES:
      return a->triangles.style == b->triangles.style;
    case BATCH_BOX:
      return a->box.style == b->box.style;
    case BATCH_ARC:
      return
        a->arc.style == b->arc.style && a->arc.mode == b->arc.mode &&
        a->arc.r1 == b->arc.r1 && a->arc.r2 == b->arc.r2 && a->arc.segments == b->arc.segments;
    case BATCH_CYLINDER:
      return
        a->cylinder.r1 == b->cylinder.r1 && a->cylinder.r2 == b->cylinder.r2 &&
        a->cylinder.capped == b->cylinder.capped && a->cylinder.segments == b->cylinder.segments;
    case BATCH_SPHERE:
      return a->sphere.segments == b->sphere.segments;
    case BATCH_MESH:
      return
        a->mesh.object == b->mesh.object && a->mesh.mode == b->mesh.mode &&
        a->mesh.rangeStart == b->mesh.rangeStart && a->mesh.rangeCount == b->mesh.rangeCount;
    default:
      return true;
  }
}

// Base

bool lovrGraphicsInit(bool gammaCorrect) {
  state.gammaCorrect = gammaCorrect;
  return false;
}

void lovrGraphicsDestroy() {
  if (!state.initialized) return;
  lovrGraphicsSetShader(NULL);
  lovrGraphicsSetFont(NULL);
  lovrGraphicsSetCanvas(NULL);
  for (usize i = 0; i < MAX_DEFAULT_SHADERS; i++) {
    lovrRelease(Shader, state.defaultShaders[i]);
  }
  for (usize i = 0; i < MAX_BUFFER_ROLES; i++) {
    lovrRelease(Buffer, state.buffers[i]);
    for (usize j = 0; j < MAX_LOCKS; j++) {
      lovrGpuDestroyLock(state.locks[i][j]);
    }
  }
  lovrRelease(Mesh, state.mesh);
  lovrRelease(Mesh, state.instancedMesh);
  lovrRelease(Buffer, state.identityBuffer);
  lovrRelease(Material, state.defaultMaterial);
  lovrRelease(Font, state.defaultFont);
  lovrGpuDestroy();
  memset(&state, 0, sizeof(state));
}

void lovrGraphicsPresent() {
  lovrGraphicsFlush();
  lovrPlatformSwapBuffers();
  lovrGpuPresent();
}

void lovrGraphicsCreateWindow(WindowFlags* flags) {
  lovrAssert(!state.initialized, "Window is already created");
  flags->srgb = state.gammaCorrect;
#ifdef EMSCRIPTEN
  flags->vsync = 1;
#else
  flags->vsync = 0;
#endif
  lovrAssert(lovrPlatformCreateWindow(flags), "Could not create window");
  lovrPlatformOnWindowClose(onCloseWindow);
  lovrPlatformOnWindowResize(onResizeWindow);
  lovrPlatformGetFramebufferSize(&state.width, &state.height);
  lovrGpuInit(state.gammaCorrect, lovrGetProcAddress);
  lovrGraphicsInitBuffers();
  lovrGraphicsReset();
  state.initialized = true;
}

u32 lovrGraphicsGetWidth() {
  return state.width;
}

u32 lovrGraphicsGetHeight() {
  return state.height;
}

f32 lovrGraphicsGetPixelDensity() {
  u32 width, framebufferWidth;
  lovrPlatformGetWindowSize(&width, NULL);
  lovrPlatformGetFramebufferSize(&framebufferWidth, NULL);
  if (width == 0 || framebufferWidth == 0) {
    return 0.f;
  } else {
    return (f32) framebufferWidth / (f32) width;
  }
}

void lovrGraphicsSetCamera(Camera* camera, bool clear) {
  lovrGraphicsFlush();

  if (state.camera.canvas && (!camera || camera->canvas != state.camera.canvas)) {
    lovrCanvasResolve(state.camera.canvas);
  }

  if (!camera) {
    memset(&state.camera, 0, sizeof(Camera));
    mat4_identity(state.camera.viewMatrix[0]);
    mat4_identity(state.camera.viewMatrix[1]);
    mat4_perspective(state.camera.projection[0], .01f, 100.f, 67.f * PI / 180.f, (f32) state.width / state.height);
    mat4_perspective(state.camera.projection[1], .01f, 100.f, 67.f * PI / 180.f, (f32) state.width / state.height);
  } else {
    state.camera = *camera;
  }

  if (clear) {
    Color background = state.backgroundColor;
    gammaCorrectColor(&background);
    lovrGpuClear(state.camera.canvas, &background, &(f32) { 1.f }, &(u8) { 0u });
  }
}

Buffer* lovrGraphicsGetIdentityBuffer() {
  return state.identityBuffer;
}

// State

void lovrGraphicsReset() {
  state.transform = 0;
  lovrGraphicsSetCamera(NULL, false);
  lovrGraphicsSetBackgroundColor((Color) { 0.f, 0.f, 0.f, 1.f });
  lovrGraphicsSetBlendMode(BLEND_ALPHA, BLEND_ALPHA_MULTIPLY);
  lovrGraphicsSetCanvas(NULL);
  lovrGraphicsSetColor((Color) { 1.f, 1.f, 1.f, 1.f });
  lovrGraphicsSetCullingEnabled(false);
  lovrGraphicsSetDefaultFilter((TextureFilter) { .mode = FILTER_TRILINEAR });
  lovrGraphicsSetDepthTest(COMPARE_LEQUAL, true);
  lovrGraphicsSetFont(NULL);
  lovrGraphicsSetLineWidth(1);
  lovrGraphicsSetPointSize(1);
  lovrGraphicsSetShader(NULL);
  lovrGraphicsSetStencilTest(COMPARE_NONE, 0);
  lovrGraphicsSetWinding(WINDING_COUNTERCLOCKWISE);
  lovrGraphicsSetWireframe(false);
  lovrGraphicsOrigin();
}

bool lovrGraphicsGetAlphaSampling() {
  return state.pipeline.alphaSampling;
}

void lovrGraphicsSetAlphaSampling(bool sample) {
  state.pipeline.alphaSampling = sample;
}

Color lovrGraphicsGetBackgroundColor() {
  return state.backgroundColor;
}

void lovrGraphicsSetBackgroundColor(Color color) {
  state.backgroundColor = color;
}

void lovrGraphicsGetBlendMode(BlendMode* mode, BlendAlphaMode* alphaMode) {
  *mode = state.pipeline.blendMode;
  *alphaMode = state.pipeline.blendAlphaMode;
}

void lovrGraphicsSetBlendMode(BlendMode mode, BlendAlphaMode alphaMode) {
  state.pipeline.blendMode = mode;
  state.pipeline.blendAlphaMode = alphaMode;
}

Canvas* lovrGraphicsGetCanvas() {
  return state.canvas;
}

void lovrGraphicsSetCanvas(Canvas* canvas) {
  if (state.canvas && canvas != state.canvas) {
    lovrCanvasResolve(state.canvas);
  }

  lovrRetain(canvas);
  lovrRelease(Canvas, state.canvas);
  state.canvas = canvas;
}

Color lovrGraphicsGetColor() {
  return state.color;
}

void lovrGraphicsSetColor(Color color) {
  state.color = color;
}

bool lovrGraphicsIsCullingEnabled() {
  return state.pipeline.culling;
}

void lovrGraphicsSetCullingEnabled(bool culling) {
  state.pipeline.culling = culling;
}

TextureFilter lovrGraphicsGetDefaultFilter() {
  return state.defaultFilter;
}

void lovrGraphicsSetDefaultFilter(TextureFilter filter) {
  state.defaultFilter = filter;
}

void lovrGraphicsGetDepthTest(CompareMode* mode, bool* write) {
  *mode = state.pipeline.depthTest;
  *write = state.pipeline.depthWrite;
}

void lovrGraphicsSetDepthTest(CompareMode mode, bool write) {
  state.pipeline.depthTest = mode;
  state.pipeline.depthWrite = write;
}

Font* lovrGraphicsGetFont() {
  if (!state.font) {
    if (!state.defaultFont) {
      Rasterizer* rasterizer = lovrRasterizerCreate(NULL, 32);
      state.defaultFont = lovrFontCreate(rasterizer);
      lovrRelease(Rasterizer, rasterizer);
    }

    lovrGraphicsSetFont(state.defaultFont);
  }

  return state.font;
}

void lovrGraphicsSetFont(Font* font) {
  lovrRetain(font);
  lovrRelease(Font, state.font);
  state.font = font;
}

bool lovrGraphicsIsGammaCorrect() {
  return state.gammaCorrect;
}

u8 lovrGraphicsGetLineWidth() {
  return state.pipeline.lineWidth;
}

void lovrGraphicsSetLineWidth(u8 width) {
  state.pipeline.lineWidth = width;
}

float lovrGraphicsGetPointSize() {
  return state.pointSize;
}

void lovrGraphicsSetPointSize(f32 size) {
  state.pointSize = size;
}

Shader* lovrGraphicsGetShader() {
  return state.shader;
}

void lovrGraphicsSetShader(Shader* shader) {
  lovrAssert(!shader || lovrShaderGetType(shader) == SHADER_GRAPHICS, "Compute shaders can not be set as the active shader");
  lovrRetain(shader);
  lovrRelease(Shader, state.shader);
  state.shader = shader;
}

void lovrGraphicsGetStencilTest(CompareMode* mode, u8* value) {
  *mode = state.pipeline.stencilMode;
  *value = state.pipeline.stencilValue;
}

void lovrGraphicsSetStencilTest(CompareMode mode, u8 value) {
  state.pipeline.stencilMode = mode;
  state.pipeline.stencilValue = value;
}

Winding lovrGraphicsGetWinding() {
  return state.pipeline.winding;
}

void lovrGraphicsSetWinding(Winding winding) {
  state.pipeline.winding = winding;
}

bool lovrGraphicsIsWireframe() {
  return state.pipeline.wireframe;
}

void lovrGraphicsSetWireframe(bool wireframe) {
#ifdef LOVR_GL
  state.pipeline.wireframe = wireframe;
#endif
}

// Transforms

void lovrGraphicsPush() {
  lovrAssert(++state.transform < MAX_TRANSFORMS, "Unbalanced matrix stack (more pushes than pops?)");
  mat4_init(state.transforms[state.transform], state.transforms[state.transform - 1]);
}

void lovrGraphicsPop() {
  lovrAssert(--state.transform >= 0, "Unbalanced matrix stack (more pops than pushes?)");
}

void lovrGraphicsOrigin() {
  mat4_identity(state.transforms[state.transform]);
}

void lovrGraphicsTranslate(vec3 translation) {
  mat4_translate(state.transforms[state.transform], translation[0], translation[1], translation[2]);
}

void lovrGraphicsRotate(quat rotation) {
  mat4_rotateQuat(state.transforms[state.transform], rotation);
}

void lovrGraphicsScale(vec3 scale) {
  mat4_scale(state.transforms[state.transform], scale[0], scale[1], scale[2]);
}

void lovrGraphicsMatrixTransform(mat4 transform) {
  mat4_multiply(state.transforms[state.transform], transform);
}

void lovrGraphicsSetProjection(mat4 projection) {
  mat4_set(state.camera.projection[0], projection);
  mat4_set(state.camera.projection[1], projection);
}

// Rendering

void lovrGraphicsClear(Color* color, f32* depth, u8* stencil) {
  if (color) gammaCorrectColor(color);
  if (color || depth || stencil) lovrGraphicsFlush();
  lovrGpuClear(state.canvas ? state.canvas : state.camera.canvas, color, depth, stencil);
}

void lovrGraphicsDiscard(bool color, bool depth, bool stencil) {
  if (color || depth || stencil) lovrGraphicsFlush();
  lovrGpuDiscard(state.canvas ? state.canvas : state.camera.canvas, color, depth, stencil);
}

void lovrGraphicsBatch(BatchRequest* req) {

  // Resolve objects
  Canvas* canvas = state.canvas ? state.canvas : state.camera.canvas;
  Shader* shader = state.shader ? state.shader : (state.defaultShaders[req->shader] ? state.defaultShaders[req->shader] : (state.defaultShaders[req->shader] = lovrShaderCreateDefault(req->shader)));
  Pipeline* pipeline = req->pipeline ? req->pipeline : &state.pipeline;
  Material* material = req->material ? req->material : (state.defaultMaterial ? state.defaultMaterial : (state.defaultMaterial = lovrMaterialCreate()));

  if (!req->material) {
    lovrMaterialSetTexture(material, TEXTURE_DIFFUSE, req->diffuseTexture);
    lovrMaterialSetTexture(material, TEXTURE_ENVIRONMENT_MAP, req->environmentMap);
  }

  if (req->type == BATCH_MESH) {
    f32* pose = req->params.mesh.pose ? req->params.mesh.pose : (f32[16]) MAT4_IDENTITY;
    usize count = req->params.mesh.pose ? (MAX_BONES * 16) : 16;
    lovrShaderSetMatrices(shader, "lovrPose", pose, 0, count);
  }

  // Try to find an existing batch to use
  Batch* batch = NULL;
  if (req->type != BATCH_MESH || req->params.mesh.instances == 1) {
    for (i32 i = state.batchCount - 1; i >= 0; i--) {
      Batch* b = &state.batches[i];

      if (b->count >= MAX_DRAWS) { continue; }
      if (!areBatchParamsEqual(req->type, b->type, &req->params, &b->params)) { continue; }
      if (b->canvas == canvas && b->shader == shader && !memcmp(&b->pipeline, pipeline, sizeof(Pipeline)) && b->material == material) {
        batch = b;
        break;
      }

      // Draws can't be reordered when blending is on, depth test is off, or either of the batches
      // are streaming their vertices (since buffers are append-only)
      if (b->pipeline.blendMode != BLEND_NONE || pipeline->blendMode != BLEND_NONE) { break; }
      if (b->pipeline.depthTest == COMPARE_NONE || pipeline->depthTest == COMPARE_NONE) { break; }
      if (!b->instanced || !req->instanced) { break; }
    }
  }

  if (req->vertexCount > 0 && (!req->instanced || !batch)) {
    *(req->vertices) = lovrGraphicsMapBuffer(STREAM_VERTEX, req->vertexCount);
    u8* ids = lovrGraphicsMapBuffer(STREAM_DRAW_ID, req->vertexCount);
    memset(ids, batch ? batch->count : 0, req->vertexCount * sizeof(u8));

    if (req->indexCount > 0) {
      *(req->indices) = lovrGraphicsMapBuffer(STREAM_INDEX, req->indexCount);
      *(req->baseVertex) = state.cursors[STREAM_VERTEX];
    }

    // The buffer mapping here could have triggered a flush, so if we were hoping to batch with
    // something but the batch count is zero now, we just start a new batch.  Maybe there's a better
    // way to detect this.
    if (batch && state.batchCount == 0) {
      batch = NULL;
    }
  }

  // Start a new batch
  if (!batch) {
    f32* transforms = lovrGraphicsMapBuffer(STREAM_TRANSFORM, MAX_DRAWS);
    Color* colors = lovrGraphicsMapBuffer(STREAM_COLOR, MAX_DRAWS);

    batch = &state.batches[state.batchCount++];
    *batch = (Batch) {
      .type = req->type,
      .params = req->params,
      .drawMode = req->drawMode,
      .canvas = canvas,
      .shader = shader,
      .pipeline = *pipeline,
      .material = material,
      .transforms = transforms,
      .colors = colors,
      .instanced = req->instanced
    };

    for (u32 i = 0; i < MAX_BUFFER_ROLES; i++) {
      batch->cursors[i].start = state.cursors[i];
    }

    batch->cursors[STREAM_TRANSFORM].count = MAX_DRAWS;
    batch->cursors[STREAM_COLOR].count = MAX_DRAWS;
    state.cursors[STREAM_TRANSFORM] += MAX_DRAWS;
    state.cursors[STREAM_COLOR] += MAX_DRAWS;
  }

  // Transform
  if (req->transform) {
    f32 transform[16];
    mat4_multiply(mat4_init(transform, state.transforms[state.transform]), req->transform);
    memcpy(&batch->transforms[16 * batch->count], transform, 16 * sizeof(f32));
  } else {
    memcpy(&batch->transforms[16 * batch->count], state.transforms[state.transform], 16 * sizeof(f32));
  }

  // Color
  Color color = state.color;
  gammaCorrectColor(&color);
  batch->colors[batch->count] = color;

  if (!req->instanced || batch->count == 0) {
    batch->cursors[STREAM_VERTEX].count += req->vertexCount;
    batch->cursors[STREAM_INDEX].count += req->indexCount;
    batch->cursors[STREAM_DRAW_ID].count += req->vertexCount;

    state.cursors[STREAM_VERTEX] += req->vertexCount;
    state.cursors[STREAM_INDEX] += req->indexCount;
    state.cursors[STREAM_DRAW_ID] += req->vertexCount;
  }

  batch->count++;
}

void lovrGraphicsFlush() {
  if (state.batchCount == 0) {
    return;
  }

  // Prevent infinite flushing >_>
  u8 batchCount = state.batchCount;
  state.batchCount = 0;

  for (u8 b = 0; b < batchCount; b++) {
    Batch* batch = &state.batches[b];
    BatchParams* params = &batch->params;
    Mesh* mesh = batch->type == BATCH_MESH ? params->mesh.object : (batch->instanced ? state.instancedMesh : state.mesh);
    u32 instances = batch->instanced ? batch->count : 1;

    // Flush buffers
    for (usize i = 0; i < MAX_BUFFER_ROLES; i++) {
      if (batch->cursors[i].count > 0) {
        usize stride = BUFFER_STRIDES[i];
        lovrBufferFlushRange(state.buffers[i], batch->cursors[i].start * stride, batch->cursors[i].count * stride);
      }
    }

    // Bind UBOs
    lovrShaderSetBlock(batch->shader, "lovrModelBlock", state.buffers[STREAM_TRANSFORM], batch->cursors[STREAM_TRANSFORM].start * BUFFER_STRIDES[STREAM_TRANSFORM], MAX_DRAWS * BUFFER_STRIDES[STREAM_TRANSFORM], ACCESS_READ);
    lovrShaderSetBlock(batch->shader, "lovrColorBlock", state.buffers[STREAM_COLOR], batch->cursors[STREAM_COLOR].start * BUFFER_STRIDES[STREAM_COLOR], MAX_DRAWS * BUFFER_STRIDES[STREAM_COLOR], ACCESS_READ);

    // Uniforms
    lovrMaterialBind(batch->material, batch->shader);
    lovrShaderSetMatrices(batch->shader, "lovrViews", state.camera.viewMatrix[0], 0, 32);
    lovrShaderSetMatrices(batch->shader, "lovrProjections", state.camera.projection[0], 0, 32);

    if (batch->drawMode == DRAW_POINTS) {
      lovrShaderSetFloats(batch->shader, "lovrPointSize", &state.pointSize, 0, 1);
    }

    u32 rangeStart, rangeCount;
    if (batch->type == BATCH_MESH) {
      rangeStart = params->mesh.rangeStart;
      rangeCount = params->mesh.rangeCount;
      if (params->mesh.instances > 1) {
        lovrMeshSetAttributeEnabled(mesh, "lovrDrawID", false);
        instances = params->mesh.instances;
      } else {
        lovrMeshSetAttributeEnabled(mesh, "lovrDrawID", true);
        instances = batch->count;
      }
    } else {
      bool indexed = batch->cursors[STREAM_INDEX].count > 0;
      rangeStart = batch->cursors[indexed ? STREAM_INDEX : STREAM_VERTEX].start;
      rangeCount = batch->cursors[indexed ? STREAM_INDEX : STREAM_VERTEX].count;
      if (indexed) {
        lovrMeshSetIndexBuffer(mesh, state.buffers[STREAM_INDEX], BUFFER_COUNTS[STREAM_INDEX], sizeof(u16), 0);
      } else {
        lovrMeshSetIndexBuffer(mesh, NULL, 0, 0, 0);
      }
    }

    // Draw!
    lovrGpuDraw(&(DrawCommand) {
      .mesh = mesh,
      .shader = batch->shader,
      .canvas = batch->canvas,
      .pipeline = batch->pipeline,
      .drawMode = batch->drawMode,
      .instances = instances,
      .rangeStart = rangeStart,
      .rangeCount = rangeCount,
      .width = batch->canvas ? lovrCanvasGetWidth(batch->canvas) : state.width,
      .height = batch->canvas ? lovrCanvasGetHeight(batch->canvas) : state.height,
      .stereo = batch->type != BATCH_FILL && (batch->canvas ? lovrCanvasIsStereo(batch->canvas) : state.camera.stereo)
    });

    // Pop lock and drop it
    for (usize i = 0; i < MAX_BUFFER_ROLES; i++) {
      if (batch->cursors[i].count > 0) {
        usize lockSize = BUFFER_COUNTS[i] / MAX_LOCKS;
        usize start = batch->cursors[i].start;
        usize count = batch->cursors[i].count + 1;
        usize firstLock = start / lockSize;
        usize lastLock = MIN(start + count, BUFFER_COUNTS[i] - 1) / lockSize;
        for (usize j = firstLock; j < lastLock; j++) {
          state.locks[i][j] = lovrGpuLock();
        }
      }
    }
  }
}

void lovrGraphicsFlushCanvas(Canvas* canvas) {
  for (i32 i = state.batchCount - 1; i >= 0; i--) {
    if (state.batches[i].canvas == canvas) {
      lovrGraphicsFlush();
      return;
    }
  }
}

void lovrGraphicsFlushShader(Shader* shader) {
  for (i32 i = state.batchCount - 1; i >= 0; i--) {
    if (state.batches[i].shader == shader) {
      lovrGraphicsFlush();
      return;
    }
  }
}

void lovrGraphicsFlushMaterial(Material* material) {
  for (i32 i = state.batchCount - 1; i >= 0; i--) {
    if (state.batches[i].material == material) {
      lovrGraphicsFlush();
      return;
    }
  }
}

void lovrGraphicsFlushMesh(Mesh* mesh) {
  for (i32 i = state.batchCount - 1; i >= 0; i--) {
    if (state.batches[i].type == BATCH_MESH && state.batches[i].params.mesh.object == mesh) {
      lovrGraphicsFlush();
      return;
    }
  }
}

void lovrGraphicsPoints(u32 count, f32** vertices) {
  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_POINTS,
    .drawMode = DRAW_POINTS,
    .vertexCount = count,
    .vertices = vertices
  });
}

void lovrGraphicsLine(u32 count, f32** vertices) {
  u32 indexCount = count + 1;
  u16* indices;
  u16 baseVertex;

  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_LINES,
    .drawMode = DRAW_LINE_STRIP,
    .vertexCount = count,
    .vertices = vertices,
    .indexCount = indexCount,
    .indices = &indices,
    .baseVertex = &baseVertex
  });

  indices[0] = 0xffff;
  for (u32 i = 1; i < indexCount; i++) {
    indices[i] = baseVertex + i - 1;
  }
}

void lovrGraphicsTriangle(DrawStyle style, Material* material, u32 count, f32** vertices) {
  u32 indexCount = style == STYLE_LINE ? (4 * count / 3) : 0;
  u16* indices;
  u16 baseVertex;

  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_TRIANGLES,
    .params.triangles.style = style,
    .drawMode = style == STYLE_LINE ? DRAW_LINE_LOOP : DRAW_TRIANGLES,
    .material = material,
    .vertexCount = count,
    .vertices = vertices,
    .indexCount = indexCount,
    .indices = &indices,
    .baseVertex = &baseVertex
  });

  if (style == STYLE_LINE) {
    for (u32 i = 0; i < count; i += 3) {
      *indices++ = 0xffff;
      *indices++ = baseVertex + i + 0;
      *indices++ = baseVertex + i + 1;
      *indices++ = baseVertex + i + 2;
    }
  }
}

void lovrGraphicsPlane(DrawStyle style, Material* material, mat4 transform, f32 u, f32 v, f32 w, f32 h) {
  f32* vertices = NULL;
  u16* indices = NULL;
  u16 baseVertex;

  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_PLANE,
    .params.plane.style = style,
    .drawMode = style == STYLE_LINE ? DRAW_LINE_LOOP : DRAW_TRIANGLES,
    .material = material,
    .transform = transform,
    .vertexCount = 4,
    .indexCount = style == STYLE_LINE ? 5 : 6,
    .vertices = &vertices,
    .indices = &indices,
    .baseVertex = &baseVertex
  });

  if (style == STYLE_LINE) {
    static f32 vertexData[] = {
      -.5f,  .5f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
       .5f,  .5f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
       .5f, -.5f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f,
      -.5f, -.5f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f
    };

    memcpy(vertices, vertexData, sizeof(vertexData));

    indices[0] = 0xffff;
    indices[1] = 0 + baseVertex;
    indices[2] = 1 + baseVertex;
    indices[3] = 2 + baseVertex;
    indices[4] = 3 + baseVertex;
  } else {
    f32 vertexData[] = {
      -.5f,  .5f, 0.f, 0.f, 0.f, -1.f, u, v + h,
      -.5f, -.5f, 0.f, 0.f, 0.f, -1.f, u, v,
       .5f,  .5f, 0.f, 0.f, 0.f, -1.f, u + w, v + h,
       .5f, -.5f, 0.f, 0.f, 0.f, -1.f, u + w, v
    };

    memcpy(vertices, vertexData, sizeof(vertexData));

    static u16 indexData[] = { 0, 1, 2, 2, 1, 3 };

    for (usize i = 0; i < sizeof(indexData) / sizeof(indexData[0]); i++) {
      indices[i] = indexData[i] + baseVertex;
    }
  }
}

void lovrGraphicsBox(DrawStyle style, Material* material, mat4 transform) {
  f32* vertices = NULL;
  u16* indices = NULL;
  u16 baseVertex;

  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_BOX,
    .params.box.style = style,
    .drawMode = style == STYLE_LINE ? DRAW_LINES : DRAW_TRIANGLES,
    .material = material,
    .transform = transform,
    .vertexCount = style == STYLE_LINE ? 8 : 24,
    .indexCount = style == STYLE_LINE ? 24 : 36,
    .vertices = &vertices,
    .indices = &indices,
    .baseVertex = &baseVertex,
    .instanced = true
  });

  if (vertices) {
    if (style == STYLE_LINE) {
      static f32 vertexData[] = {
        -.5f,  .5f, -.5f, 0.f, 0.f, 0.f, 0.f, 0.f, // Front
         .5f,  .5f, -.5f, 0.f, 0.f, 0.f, 0.f, 0.f,
         .5f, -.5f, -.5f, 0.f, 0.f, 0.f, 0.f, 0.f,
        -.5f, -.5f, -.5f, 0.f, 0.f, 0.f, 0.f, 0.f,
        -.5f,  .5f,  .5f, 0.f, 0.f, 0.f, 0.f, 0.f, // Back
         .5f,  .5f,  .5f, 0.f, 0.f, 0.f, 0.f, 0.f,
         .5f, -.5f,  .5f, 0.f, 0.f, 0.f, 0.f, 0.f,
        -.5f, -.5f,  .5f, 0.f, 0.f, 0.f, 0.f, 0.f
      };

      memcpy(vertices, vertexData, sizeof(vertexData));

      static u16 indexData[] = {
        0, 1, 1, 2, 2, 3, 3, 0, // Front
        4, 5, 5, 6, 6, 7, 7, 4, // Back
        0, 4, 1, 5, 2, 6, 3, 7  // Connections
      };

      for (usize i = 0; i < sizeof(indexData) / sizeof(indexData[0]); i++) {
        indices[i] = indexData[i] + baseVertex;
      }
    } else {
      static f32 vertexData[] = {
        -.5f, -.5f, -.5f,  0.f,  0.f, -1.f, 0.f, 0.f, // Front
        -.5f,  .5f, -.5f,  0.f,  0.f, -1.f, 0.f, 1.f,
         .5f, -.5f, -.5f,  0.f,  0.f, -1.f, 1.f, 0.f,
         .5f,  .5f, -.5f,  0.f,  0.f, -1.f, 1.f, 1.f,
         .5f,  .5f, -.5f,  1.f,  0.f,  0.f, 0.f, 1.f, // Right
         .5f,  .5f,  .5f,  1.f,  0.f,  0.f, 1.f, 1.f,
         .5f, -.5f, -.5f,  1.f,  0.f,  0.f, 0.f, 0.f,
         .5f, -.5f,  .5f,  1.f,  0.f,  0.f, 1.f, 0.f,
         .5f, -.5f,  .5f,  0.f,  0.f,  1.f, 0.f, 0.f, // Back
         .5f,  .5f,  .5f,  0.f,  0.f,  1.f, 0.f, 1.f,
        -.5f, -.5f,  .5f,  0.f,  0.f,  1.f, 1.f, 0.f,
        -.5f,  .5f,  .5f,  0.f,  0.f,  1.f, 1.f, 1.f,
        -.5f,  .5f,  .5f, -1.f,  0.f,  0.f, 0.f, 1.f, // Left
        -.5f,  .5f, -.5f, -1.f,  0.f,  0.f, 1.f, 1.f,
        -.5f, -.5f,  .5f, -1.f,  0.f,  0.f, 0.f, 0.f,
        -.5f, -.5f, -.5f, -1.f,  0.f,  0.f, 1.f, 0.f,
        -.5f, -.5f, -.5f,  0.f, -1.f,  0.f, 0.f, 0.f, // Bottom
         .5f, -.5f, -.5f,  0.f, -1.f,  0.f, 1.f, 0.f,
        -.5f, -.5f,  .5f,  0.f, -1.f,  0.f, 0.f, 1.f,
         .5f, -.5f,  .5f,  0.f, -1.f,  0.f, 1.f, 1.f,
        -.5f,  .5f, -.5f,  0.f,  1.f,  0.f, 0.f, 1.f, // Top
        -.5f,  .5f,  .5f,  0.f,  1.f,  0.f, 0.f, 0.f,
         .5f,  .5f, -.5f,  0.f,  1.f,  0.f, 1.f, 1.f,
         .5f,  .5f,  .5f,  0.f,  1.f,  0.f, 1.f, 0.f
      };

      memcpy(vertices, vertexData, sizeof(vertexData));

      u16 indexData[] = {
        0,  1,   2,  2,  1,  3,
        4,  5,   6,  6,  5,  7,
        8,  9,  10, 10,  9, 11,
        12, 13, 14, 14, 13, 15,
        16, 17, 18, 18, 17, 19,
        20, 21, 22, 22, 21, 23
      };

      for (usize i = 0; i < sizeof(indexData) / sizeof(indexData[0]); i++) {
        indices[i] = indexData[i] + baseVertex;
      }
    }
  }
}

void lovrGraphicsArc(DrawStyle style, ArcMode mode, Material* material, mat4 transform, f32 r1, f32 r2, u32 segments) {
  bool hasCenterPoint = false;

  if (fabsf(r1 - r2) >= 2.f * PI) {
    r1 = 0.f;
    r2 = 2.f * PI;
  } else {
    hasCenterPoint = mode == ARC_MODE_PIE;
  }

  u32 vertexCount = segments + 1 + hasCenterPoint;
  f32* vertices = NULL;

  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_ARC,
    .params.arc.r1 = r1,
    .params.arc.r2 = r2,
    .params.arc.mode = mode,
    .params.arc.style = style,
    .params.arc.segments = segments,
    .drawMode = style == STYLE_LINE ? (mode == ARC_MODE_OPEN ? DRAW_LINE_STRIP : DRAW_LINE_LOOP) : DRAW_TRIANGLE_FAN,
    .material = material,
    .transform = transform,
    .vertexCount = vertexCount,
    .vertices = &vertices,
    .instanced = true
  });

  if (vertices) {
    if (hasCenterPoint) {
      memcpy(vertices, ((f32[]) { 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, .5f, .5f }), 8 * sizeof(f32));
      vertices += 8;
    }

    f32 theta = r1;
    f32 angleShift = (r2 - r1) / (f32) segments;

    for (u32 i = 0; i <= segments; i++) {
      f32 x = cosf(theta) * .5f;
      f32 y = sinf(theta) * .5f;
      memcpy(vertices, ((f32[]) { x, y, 0.f, 0.f, 0.f, 1.f, x + .5f, 1.f - (y + .5f) }), 8 * sizeof(f32));
      vertices += 8;
      theta += angleShift;
    }
  }
}

void lovrGraphicsCircle(DrawStyle style, Material* material, mat4 transform, u32 segments) {
  lovrGraphicsArc(style, ARC_MODE_OPEN, material, transform, 0, 2.f * PI, segments);
}

void lovrGraphicsCylinder(Material* material, mat4 transform, f32 r1, f32 r2, bool capped, u32 segments) {
  f32 length = vec3_length((f32[3]) { transform[8], transform[9], transform[10] });
  r1 /= length;
  r2 /= length;

  u32 vertexCount = ((capped && r1) * (segments + 2) + (capped && r2) * (segments + 2) + 2 * (segments + 1));
  u32 indexCount = 3 * segments * ((capped && r1) + (capped && r2) + 2);
  f32* vertices = NULL;
  u16* indices = NULL;
  u16 baseVertex;

  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_CYLINDER,
    .params.cylinder.r1 = r1,
    .params.cylinder.r2 = r2,
    .params.cylinder.capped = capped,
    .params.cylinder.segments = segments,
    .drawMode = DRAW_TRIANGLES,
    .material = material,
    .transform = transform,
    .vertexCount = vertexCount,
    .indexCount = indexCount,
    .vertices = &vertices,
    .indices = &indices,
    .baseVertex = &baseVertex,
    .instanced = true
  });

  if (vertices) {
    f32* v = vertices;

    // Ring
    for (u32 i = 0; i <= segments; i++) {
      f32 theta = i * (2.f * PI) / segments;
      f32 X = cosf(theta);
      f32 Y = sinf(theta);
      memcpy(vertices, (f32[16]) {
        r1 * X, r1 * Y, -.5f, X, Y, 0.f, 0.f, 0.f,
        r2 * X, r2 * Y,  .5f, X, Y, 0.f, 0.f, 0.f
      }, 16 * sizeof(f32));
      vertices += 16;
    }

    // Top
    u32 top = (segments + 1) * 2 + baseVertex;
    if (capped && r1 != 0) {
      memcpy(vertices, (f32[8]) { 0.f, 0.f, -.5f, 0.f, 0.f, -1.f, 0.f, 0.f }, 8 * sizeof(f32));
      vertices += 8;
      for (u32 i = 0; i <= segments; i++) {
        u32 j = i * 2 * 8;
        memcpy(vertices, (f32[8]) { v[j + 0], v[j + 1], v[j + 2], 0.f, 0.f, -1.f, 0.f, 0.f }, 8 * sizeof(f32));
        vertices += 8;
      }
    }

    // Bottom
    u32 bot = (segments + 1) * 2 + (1 + segments + 1) * (capped && r1 != 0) + baseVertex;
    if (capped && r2 != 0) {
      memcpy(vertices, (f32[8]) { 0.f, 0.f, .5f, 0.f, 0.f, 1.f, 0.f, 0.f }, 8 * sizeof(f32));
      vertices += 8;
      for (u32 i = 0; i <= segments; i++) {
        u32 j = i * 2 * 8 + 8;
        memcpy(vertices, (float[8]) { v[j + 0], v[j + 1], v[j + 2], 0.f, 0.f, 1.f, 0.f, 0.f }, 8 * sizeof(f32));
        vertices += 8;
      }
    }

    // Indices
    for (u32 i = 0; i < segments; i++) {
      u16 j = 2 * i + baseVertex;
      memcpy(indices, (u16[6]) { j, j + 1, j + 2, j + 1, j + 3, j + 2 }, 6 * sizeof(u16));
      indices += 6;

      if (capped && r1 != 0.f) {
        memcpy(indices, (u16[3]) { top, top + i + 1, top + i + 2 }, 3 * sizeof(u16));
        indices += 3;
      }

      if (capped && r2 != 0.f) {
        memcpy(indices, (u16[3]) { bot, bot + i + 1, bot + i + 2 }, 3 * sizeof(u16));
        indices += 3;
      }
    }
  }
}

void lovrGraphicsSphere(Material* material, mat4 transform, u32 segments) {
  f32* vertices = NULL;
  u16* indices = NULL;
  u16 baseVertex;

  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_SPHERE,
    .params.sphere.segments = segments,
    .drawMode = DRAW_TRIANGLES,
    .material = material,
    .transform = transform,
    .vertexCount = (segments + 1) * (segments + 1),
    .indexCount = segments * segments * 6,
    .vertices = &vertices,
    .indices = &indices,
    .baseVertex = &baseVertex,
    .instanced = true
  });

  if (vertices) {
    for (u32 i = 0; i <= segments; i++) {
      f32 v = i / (f32) segments;
      f32 sinV = sinf(v * PI);
      f32 cosV = cosf(v * PI);
      for (u32 k = 0; k <= segments; k++) {
        f32 u = k / (f32) segments;
        f32 x = sinf(u * 2.f * PI) * sinV;
        f32 y = cosV;
        f32 z = -cosf(u * 2.f * PI) * sinV;
        memcpy(vertices, ((f32[8]) { x, y, z, x, y, z, u, 1.f - v }), 8 * sizeof(f32));
        vertices += 8;
      }
    }

    for (u32 i = 0; i < segments; i++) {
      u16 offset0 = i * (segments + 1) + baseVertex;
      u16 offset1 = (i + 1) * (segments + 1) + baseVertex;
      for (u32 j = 0; j < segments; j++) {
        u16 i0 = offset0 + j;
        u16 i1 = offset1 + j;
        memcpy(indices, ((u16[]) { i0, i1, i0 + 1, i1, i1 + 1, i0 + 1 }), 6 * sizeof(u16));
        indices += 6;
      }
    }
  }
}

void lovrGraphicsSkybox(Texture* texture, f32 angle, f32 ax, f32 ay, f32 az) {
  TextureType type = lovrTextureGetType(texture);
  lovrAssert(type == TEXTURE_CUBE || type == TEXTURE_2D, "Only 2D and cube textures can be used as skyboxes");

  Pipeline pipeline = state.pipeline;
  pipeline.winding = WINDING_COUNTERCLOCKWISE;

  f32 transform[16] = MAT4_IDENTITY;
  mat4_rotate(transform, angle, ax, ay, az);

  f32* vertices = NULL;

  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_SKYBOX,
    .drawMode = DRAW_TRIANGLE_STRIP,
    .shader = type == TEXTURE_CUBE ? SHADER_CUBE : SHADER_PANO,
    .pipeline = &pipeline,
    .transform = transform,
    .diffuseTexture = type == TEXTURE_2D ? texture : NULL,
    .environmentMap = type == TEXTURE_CUBE ? texture : NULL,
    .vertexCount = 4,
    .vertices = &vertices,
    .instanced = true
  });

  if (vertices) {
    static f32 vertexData[] = {
      -1.f,  1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
      -1.f, -1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
       1.f,  1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f,
       1.f, -1.f, 1.f, 0.f, 0.f, 0.f, 0.f, 0.f
    };

    memcpy(vertices, vertexData, sizeof(vertexData));
  }
}

void lovrGraphicsPrint(const char* str, usize length, mat4 transform, f32 wrap, HorizontalAlign halign, VerticalAlign valign) {
  f32 width;
  u32 lineCount;
  u32 glyphCount;
  Font* font = lovrGraphicsGetFont();
  lovrFontMeasure(font, str, length, wrap, &width, &lineCount, &glyphCount);

  f32 scale = 1.f / font->pixelDensity;
  f32 offsetY = ((lineCount + 1) * font->rasterizer->height * font->lineHeight) * (valign / 2.f) * (font->flip ? -1 : 1);
  mat4_scale(transform, scale, scale, scale);
  mat4_translate(transform, 0.f, offsetY, 0.f);

  Pipeline pipeline = state.pipeline;
  pipeline.blendMode = pipeline.blendMode == BLEND_NONE ? BLEND_ALPHA : pipeline.blendMode;

  f32* vertices;
  u16* indices;
  u16 baseVertex;
  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_TEXT,
    .drawMode = DRAW_TRIANGLES,
    .shader = SHADER_FONT,
    .pipeline = &pipeline,
    .transform = transform,
    .diffuseTexture = font->texture,
    .vertexCount = glyphCount * 4,
    .indexCount = glyphCount * 6,
    .vertices = &vertices,
    .indices = &indices,
    .baseVertex = &baseVertex
  });

  lovrFontRender(font, str, length, wrap, halign, vertices, indices, baseVertex);
}

void lovrGraphicsFill(Texture* texture, f32 u, f32 v, f32 w, f32 h) {
  Pipeline pipeline = state.pipeline;
  pipeline.depthTest = COMPARE_NONE;
  pipeline.depthWrite = false;

  f32* vertices = NULL;

  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_FILL,
    .params.fill = { .u = u, .v = v, .w = w, .h = h },
    .drawMode = DRAW_TRIANGLE_STRIP,
    .shader = SHADER_FILL,
    .pipeline = &pipeline,
    .diffuseTexture = texture,
    .vertexCount = 4,
    .vertices = &vertices
  });

  if (vertices) {
    memcpy(vertices, (f32[32]) {
      -1.f,  1.f, 0.f, 0.f, 0.f, 0.f, u, v + h,
      -1.f, -1.f, 0.f, 0.f, 0.f, 0.f, u, v,
       1.f,  1.f, 0.f, 0.f, 0.f, 0.f, u + w, v + h,
       1.f, -1.f, 0.f, 0.f, 0.f, 0.f, u + w, v
    }, 32 * sizeof(f32));
  }
}
