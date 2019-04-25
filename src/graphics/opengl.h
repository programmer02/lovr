#if LOVR_WEBGL
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <GL/gl.h>
#include <GL/glext.h>
#else
#include "lib/glad/glad.h"
#endif

#pragma once

#define GPU_BUFFER_FIELDS \
  GLsync lock; \
  u32 id; \
  u8 incoherent;

#define GPU_CANVAS_FIELDS \
  u32 framebuffer; \
  u32 resolveBuffer; \
  u32 depthBuffer; \
  bool immortal;

#define GPU_MESH_FIELDS \
  u32 vao; \
  u32 ibo;

#define GPU_SHADER_FIELDS \
  u32 program;

#define GPU_TEXTURE_FIELDS \
  GLuint id; \
  GLuint msaaId; \
  GLenum target; \
  u8 incoherent;
