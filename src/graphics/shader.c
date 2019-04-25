#include "graphics/shader.h"
#include "graphics/graphics.h"
#include "graphics/buffer.h"
#include "math/math.h"
#include "resources/shaders.h"
#include "lib/err.h"
#include <math.h>

static usize getUniformTypeLength(const Uniform* uniform) {
  usize size = 0;

  if (uniform->count > 1) {
    size += 2 + floor(log10(uniform->count)) + 1; // "[count]"
  }

  switch (uniform->type) {
    case UNIFORM_MATRIX: size += 4; break;
    case UNIFORM_FLOAT: size += uniform->components == 1 ? 5 : 4; break;
    case UNIFORM_INT: size += uniform->components == 1 ? 3 : 5; break;
    default: break;
  }

  return size;
}

static const char* getUniformTypeName(const Uniform* uniform) {
  switch (uniform->type) {
    case UNIFORM_FLOAT:
      switch (uniform->components) {
        case 1: return "float";
        case 2: return "vec2";
        case 3: return "vec3";
        case 4: return "vec4";
      }
      break;

    case UNIFORM_INT:
      switch (uniform->components) {
        case 1: return "int";
        case 2: return "ivec2";
        case 3: return "ivec3";
        case 4: return "ivec4";
      }
      break;

    case UNIFORM_MATRIX:
      switch (uniform->components) {
        case 2: return "mat2";
        case 3: return "mat3";
        case 4: return "mat4";
      }
      break;

    default: break;
  }

  lovrThrow("Unreachable");
  return "";
}

Shader* lovrShaderInitDefault(Shader* shader, DefaultShader type) {
  switch (type) {
    case SHADER_DEFAULT: return lovrShaderInitGraphics(shader, NULL, NULL);
    case SHADER_CUBE: return lovrShaderInitGraphics(shader, lovrCubeVertexShader, lovrCubeFragmentShader);
    case SHADER_PANO: return lovrShaderInitGraphics(shader, lovrCubeVertexShader, lovrPanoFragmentShader);
    case SHADER_FONT: return lovrShaderInitGraphics(shader, NULL, lovrFontFragmentShader);
    case SHADER_FILL: return lovrShaderInitGraphics(shader, lovrFillVertexShader, NULL);
    default: lovrThrow("Unknown default shader type"); return NULL;
  }
}

ShaderType lovrShaderGetType(Shader* shader) {
  return shader->type;
}

u32* lovrShaderGetAttributeLocation(Shader* shader, const char* name) {
  return map_get(&shader->attributes, name);
}

bool lovrShaderHasUniform(Shader* shader, const char* name) {
  return map_get(&shader->uniformMap, name) != NULL;
}

const Uniform* lovrShaderGetUniform(Shader* shader, const char* name) {
  u32* index = map_get(&shader->uniformMap, name);
  return index ? &shader->uniforms.data[*index] : false;
}

static void lovrShaderSetUniform(Shader* shader, const char* name, UniformType type, void* data, usize start, usize count, usize size, const char* debug) {
  u32* index = map_get(&shader->uniformMap, name);
  if (!index) {
    return;
  }

  Uniform* uniform = &shader->uniforms.data[*index];
  lovrAssert(uniform->type == type, "Unable to send %ss to uniform %s", debug, name);
  lovrAssert((start + count) * size <= uniform->size, "Too many %ss for uniform %s, maximum is %d", debug, name, uniform->size / size);

  void* dest = uniform->value.bytes + start * size;
  if (memcmp(dest, data, count * size)) {
    lovrGraphicsFlushShader(shader);
    memcpy(dest, data, count * size);
    uniform->dirty = true;
  }
}

void lovrShaderSetFloats(Shader* shader, const char* name, f32* data, usize start, usize count) {
  lovrShaderSetUniform(shader, name, UNIFORM_FLOAT, data, start, count, sizeof(f32), "float");
}

void lovrShaderSetInts(Shader* shader, const char* name, i32* data, usize start, usize count) {
  lovrShaderSetUniform(shader, name, UNIFORM_INT, data, start, count, sizeof(i32), "int");
}

void lovrShaderSetMatrices(Shader* shader, const char* name, f32* data, usize start, usize count) {
  lovrShaderSetUniform(shader, name, UNIFORM_MATRIX, data, start, count, sizeof(f32), "float");
}

void lovrShaderSetTextures(Shader* shader, const char* name, Texture** data, usize start, usize count) {
  lovrShaderSetUniform(shader, name, UNIFORM_SAMPLER, data, start, count, sizeof(Texture*), "texture");
}

void lovrShaderSetImages(Shader* shader, const char* name, Image* data, usize start, usize count) {
  lovrShaderSetUniform(shader, name, UNIFORM_IMAGE, data, start, count, sizeof(Image), "image");
}

void lovrShaderSetColor(Shader* shader, const char* name, Color color) {
  if (lovrGraphicsIsGammaCorrect()) {
    color.r = lovrMathGammaToLinear(color.r);
    color.g = lovrMathGammaToLinear(color.g);
    color.b = lovrMathGammaToLinear(color.b);
  }

  lovrShaderSetUniform(shader, name, UNIFORM_FLOAT, &color.r, 0, 4, sizeof(f32), "float");
}

void lovrShaderSetBlock(Shader* shader, const char* name, Buffer* buffer, usize offset, usize size, UniformAccess access) {
  u32* id = map_get(&shader->blockMap, name);
  if (!id) return;

  u32 type = *id & 1;
  u32 index = *id >> 1;
  UniformBlock* block = &shader->blocks[type].data[index];

  if (block->source != buffer || block->offset != offset || block->size != size) {
    lovrGraphicsFlushShader(shader);
    lovrRetain(buffer);
    lovrRelease(Buffer, block->source);
    block->access = access;
    block->source = buffer;
    block->offset = offset;
    block->size = size;
  }
}

// ShaderBlock

// Calculates uniform size and byte offsets using std140 rules, returning the total buffer size
usize lovrShaderComputeUniformLayout(vec_uniform_t* uniforms) {
  usize size = 0;
  Uniform* uniform; i32 i;
  vec_foreach_ptr(uniforms, uniform, i) {
    usize align;
    if (uniform->count > 1 || uniform->type == UNIFORM_MATRIX) {
      align = 16 * (uniform->type == UNIFORM_MATRIX ? uniform->components : 1);
      uniform->size = align * uniform->count;
    } else {
      align = (uniform->components + (uniform->components == 3)) * 4;
      uniform->size = uniform->components * 4;
    }
    uniform->offset = (size + (align - 1)) & -align;
    size = uniform->offset + uniform->size;
  }
  return size;
}

ShaderBlock* lovrShaderBlockInit(ShaderBlock* block, BlockType type, Buffer* buffer, vec_uniform_t* uniforms) {
  vec_init(&block->uniforms);
  map_init(&block->uniformMap);

  Uniform* uniform; i32 i;
  vec_extend(&block->uniforms, uniforms);
  vec_foreach_ptr(&block->uniforms, uniform, i) {
    map_set(&block->uniformMap, uniform->name, i);
  }

  block->type = type;
  block->buffer = buffer;
  lovrRetain(buffer);
  return block;
}

void lovrShaderBlockDestroy(void* ref) {
  ShaderBlock* block = ref;
  lovrRelease(Buffer, block->buffer);
  vec_deinit(&block->uniforms);
  map_deinit(&block->uniformMap);
}

BlockType lovrShaderBlockGetType(ShaderBlock* block) {
  return block->type;
}

// TODO use sds!
char* lovrShaderBlockGetShaderCode(ShaderBlock* block, const char* blockName, usize* length) {

  // Calculate
  usize size = 0;
  usize tab = 2;
  size += 15; // "layout(std140) "
  size += block->type == BLOCK_UNIFORM ? 7 : 6; // "uniform" || "buffer"
  size += 1; // " "
  size += strlen(blockName);
  size += 3; // " {\n"
  for (int i = 0; i < block->uniforms.length; i++) {
    size += tab;
    size += getUniformTypeLength(&block->uniforms.data[i]);
    size += 1; // " "
    size += strlen(block->uniforms.data[i].name);
    size += 2; // ";\n"
  }
  size += 3; // "};\n"

  // Allocate
  char* code = malloc(size + 1);
  lovrAssert(code, "Out of memory");

  // Concatenate
  char* s = code;
  s += sprintf(s, "layout(std140) %s %s {\n", block->type == BLOCK_UNIFORM ? "uniform" : "buffer", blockName);
  for (int i = 0; i < block->uniforms.length; i++) {
    const Uniform* uniform = &block->uniforms.data[i];
    if (uniform->count > 1) {
      s += sprintf(s, "  %s %s[%d];\n", getUniformTypeName(uniform), uniform->name, uniform->count);
    } else {
      s += sprintf(s, "  %s %s;\n", getUniformTypeName(uniform), uniform->name);
    }
  }
  s += sprintf(s, "};\n");
  *s = '\0';

  *length = size;
  return code;
}

const Uniform* lovrShaderBlockGetUniform(ShaderBlock* block, const char* name) {
  u32* index = map_get(&block->uniformMap, name);
  return index ? &block->uniforms.data[*index] : NULL;
}

Buffer* lovrShaderBlockGetBuffer(ShaderBlock* block) {
  return block->buffer;
}
