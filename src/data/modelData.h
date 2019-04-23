#include "util.h"
#include "types.h"

#pragma once

#define MAX_BONES 48

struct TextureData;
struct Blob;

typedef enum {
  ATTR_POSITION,
  ATTR_NORMAL,
  ATTR_TEXCOORD,
  ATTR_COLOR,
  ATTR_TANGENT,
  ATTR_BONES,
  ATTR_WEIGHTS,
  MAX_DEFAULT_ATTRIBUTES
} DefaultAttribute;

typedef enum {
  DRAW_POINTS,
  DRAW_LINES,
  DRAW_LINE_LOOP,
  DRAW_LINE_STRIP,
  DRAW_TRIANGLES,
  DRAW_TRIANGLE_STRIP,
  DRAW_TRIANGLE_FAN
} DrawMode;

typedef enum {
  FILTER_NEAREST,
  FILTER_BILINEAR,
  FILTER_TRILINEAR,
  FILTER_ANISOTROPIC
} FilterMode;

typedef struct {
  FilterMode mode;
  f32 anisotropy;
} TextureFilter;

typedef enum {
  WRAP_CLAMP,
  WRAP_REPEAT,
  WRAP_MIRRORED_REPEAT
} WrapMode;

typedef struct {
  WrapMode s;
  WrapMode t;
  WrapMode r;
} TextureWrap;

typedef enum {
  SCALAR_METALNESS,
  SCALAR_ROUGHNESS,
  MAX_MATERIAL_SCALARS
} MaterialScalar;

typedef enum {
  COLOR_DIFFUSE,
  COLOR_EMISSIVE,
  MAX_MATERIAL_COLORS
} MaterialColor;

typedef enum {
  TEXTURE_DIFFUSE,
  TEXTURE_EMISSIVE,
  TEXTURE_METALNESS,
  TEXTURE_ROUGHNESS,
  TEXTURE_OCCLUSION,
  TEXTURE_NORMAL,
  TEXTURE_ENVIRONMENT_MAP,
  MAX_MATERIAL_TEXTURES
} MaterialTexture;

typedef enum {
  SMOOTH_STEP,
  SMOOTH_LINEAR,
  SMOOTH_CUBIC
} SmoothMode;

typedef enum {
  PROP_TRANSLATION,
  PROP_ROTATION,
  PROP_SCALE,
} AnimationProperty;

typedef enum { I8, U8, I16, U16, I32, U32, F32 } AttributeType;

typedef union {
  void* raw;
  i8* bytes;
  u8* ubytes;
  i16* shorts;
  u16* ushorts;
  i32* ints;
  u32* uints;
  f32* floats;
} AttributeData;

typedef struct {
  char* data;
  usize size;
  usize stride;
} ModelBuffer;

typedef struct {
  u32 offset;
  u32 buffer;
  u32 count;
  AttributeType type;
  u32 components : 3;
  u32 normalized : 1;
  u32 matrix : 1;
  u32 hasMin : 1;
  u32 hasMax : 1;
  f32 min[4];
  f32 max[4];
} ModelAttribute;

typedef struct {
  u32 nodeIndex;
  AnimationProperty property;
  SmoothMode smoothing;
  u32 keyframeCount;
  f32* times;
  f32* data;
} ModelAnimationChannel;

typedef struct {
  const char* name;
  ModelAnimationChannel* channels;
  u32 channelCount;
  f32 duration;
} ModelAnimation;

typedef struct {
  u32 imageIndex;
  TextureFilter filter;
  TextureWrap wrap;
} ModelTexture;

typedef struct {
  f32 scalars[MAX_MATERIAL_SCALARS];
  Color colors[MAX_MATERIAL_COLORS];
  u32 textures[MAX_MATERIAL_TEXTURES];
  TextureFilter filters[MAX_MATERIAL_TEXTURES];
  TextureWrap wraps[MAX_MATERIAL_TEXTURES];
} ModelMaterial;

typedef struct {
  ModelAttribute* attributes[MAX_DEFAULT_ATTRIBUTES];
  ModelAttribute* indices;
  DrawMode mode;
  u32 material;
} ModelPrimitive;

typedef struct {
  f32 transform[16];
  u32* children;
  u32 childCount;
  u32 primitiveIndex;
  u32 primitiveCount;
  u32 skin;
} ModelNode;

typedef struct {
  u32* joints;
  u32 jointCount;
  f32* inverseBindMatrices;
} ModelSkin;

typedef struct ModelData {
  Ref ref;
  void* data;
  struct Blob** blobs;
  ModelBuffer* buffers;
  struct TextureData** textures;
  ModelMaterial* materials;
  ModelAttribute* attributes;
  ModelPrimitive* primitives;
  ModelAnimation* animations;
  ModelSkin* skins;
  ModelNode* nodes;
  u32 rootNode;

  u32 blobCount;
  u32 bufferCount;
  u32 textureCount;
  u32 materialCount;
  u32 attributeCount;
  u32 primitiveCount;
  u32 animationCount;
  u32 skinCount;
  u32 nodeCount;

  ModelAnimationChannel* channels;
  u32* children;
  u32* joints;
  char* chars;
  u32 channelCount;
  u32 childCount;
  u32 jointCount;
  u32 charCount;
} ModelData;

ModelData* lovrModelDataInit(ModelData* model, struct Blob* blob);
#define lovrModelDataCreate(...) lovrModelDataInit(lovrAlloc(ModelData), __VA_ARGS__)
ModelData* lovrModelDataInitGltf(ModelData* model, struct Blob* blob);
ModelData* lovrModelDataInitObj(ModelData* model, struct Blob* blob);
void lovrModelDataDestroy(void* ref);
void lovrModelDataAllocate(ModelData* model);
