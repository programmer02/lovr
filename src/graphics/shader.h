#include "util.h"
#include "types.h"
#include "graphics/texture.h"
#include "graphics/opengl.h"
#include "lib/map/map.h"
#include "lib/vec/vec.h"

#pragma once

#define LOVR_MAX_UNIFORM_LENGTH 64
#define LOVR_MAX_ATTRIBUTE_LENGTH 64

struct Buffer;
struct Texture;

typedef enum {
  ACCESS_READ,
  ACCESS_WRITE,
  ACCESS_READ_WRITE
} UniformAccess;

typedef enum {
  BLOCK_UNIFORM,
  BLOCK_COMPUTE
} BlockType;

typedef enum {
  UNIFORM_FLOAT,
  UNIFORM_MATRIX,
  UNIFORM_INT,
  UNIFORM_SAMPLER,
  UNIFORM_IMAGE
} UniformType;

typedef enum {
  SHADER_GRAPHICS,
  SHADER_COMPUTE
} ShaderType;

typedef enum {
  SHADER_DEFAULT,
  SHADER_CUBE,
  SHADER_PANO,
  SHADER_FONT,
  SHADER_FILL,
  MAX_DEFAULT_SHADERS
} DefaultShader;

typedef struct {
  struct Texture* texture;
  u32 slice;
  u32 mipmap;
  UniformAccess access;
} Image;

typedef struct {
  char name[LOVR_MAX_UNIFORM_LENGTH];
  UniformType type;
  u32 components;
  u32 count;
  u32 location;
  u32 offset;
  usize size;
  union {
    void* data;
    char* bytes;
    i32* ints;
    f32* floats;
    struct Texture** textures;
    Image* images;
  } value;
  TextureType textureType;
  u32 baseSlot;
  bool image;
  bool dirty;
} Uniform;

typedef vec_t(Uniform) vec_uniform_t;

typedef struct {
  Ref ref;
  BlockType type;
  vec_uniform_t uniforms;
  map_t(u32) uniformMap;
  struct Buffer* buffer;
} ShaderBlock;

typedef struct {
  vec_uniform_t uniforms;
  UniformAccess access;
  struct Buffer* source;
  usize offset;
  usize size;
  u32 slot;
} UniformBlock;

typedef vec_t(UniformBlock) vec_block_t;

typedef struct Shader {
  Ref ref;
  ShaderType type;
  vec_uniform_t uniforms;
  vec_block_t blocks[2];
  map_t(u32) attributes;
  map_t(u32) uniformMap;
  map_t(u32) blockMap;
  GPU_SHADER_FIELDS
} Shader;

// Shader

Shader* lovrShaderInitGraphics(Shader* shader, const char* vertexSource, const char* fragmentSource);
Shader* lovrShaderInitCompute(Shader* shader, const char* source);
Shader* lovrShaderInitDefault(Shader* shader, DefaultShader type);
#define lovrShaderCreateGraphics(...) lovrShaderInitGraphics(lovrAlloc(Shader), __VA_ARGS__)
#define lovrShaderCreateCompute(...) lovrShaderInitCompute(lovrAlloc(Shader), __VA_ARGS__)
#define lovrShaderCreateDefault(...) lovrShaderInitDefault(lovrAlloc(Shader), __VA_ARGS__)
void lovrShaderDestroy(void* ref);
ShaderType lovrShaderGetType(Shader* shader);
u32* lovrShaderGetAttributeLocation(Shader* shader, const char* name);
bool lovrShaderHasUniform(Shader* shader, const char* name);
const Uniform* lovrShaderGetUniform(Shader* shader, const char* name);
void lovrShaderSetFloats(Shader* shader, const char* name, f32* data, usize start, usize count);
void lovrShaderSetInts(Shader* shader, const char* name, i32* data, usize start, usize count);
void lovrShaderSetMatrices(Shader* shader, const char* name, f32* data, usize start, usize count);
void lovrShaderSetTextures(Shader* shader, const char* name, struct Texture** data, usize start, usize count);
void lovrShaderSetImages(Shader* shader, const char* name, Image* data, usize start, usize count);
void lovrShaderSetColor(Shader* shader, const char* name, Color color);
void lovrShaderSetBlock(Shader* shader, const char* name, struct Buffer* buffer, usize offset, usize size, UniformAccess access);

// ShaderBlock

usize lovrShaderComputeUniformLayout(vec_uniform_t* uniforms);

ShaderBlock* lovrShaderBlockInit(ShaderBlock* block, BlockType type, struct Buffer* buffer, vec_uniform_t* uniforms);
#define lovrShaderBlockCreate(...) lovrShaderBlockInit(lovrAlloc(ShaderBlock), __VA_ARGS__)
void lovrShaderBlockDestroy(void* ref);
BlockType lovrShaderBlockGetType(ShaderBlock* block);
char* lovrShaderBlockGetShaderCode(ShaderBlock* block, const char* blockName, usize* length);
const Uniform* lovrShaderBlockGetUniform(ShaderBlock* block, const char* name);
struct Buffer* lovrShaderBlockGetBuffer(ShaderBlock* block);
