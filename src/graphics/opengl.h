#if LOVR_WEBGL
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <GL/gl.h>
#include <GL/glext.h>
#else
#include "lib/glad/glad.h"
#endif

#pragma once

#define GPU_MESH_FIELDS \
  u32 vao; \
  u32 ibo;

#define GPU_SHADER_FIELDS \
  u32 program;
