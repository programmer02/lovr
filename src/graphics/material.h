#include "util.h"
#include "types.h"
#include "data/modelData.h"

#pragma once

struct Texture;
struct Shader;

typedef struct Material {
  Ref ref;
  f32 scalars[MAX_MATERIAL_SCALARS];
  Color colors[MAX_MATERIAL_COLORS];
  struct Texture* textures[MAX_MATERIAL_TEXTURES];
  f32 transform[9];
} Material;

Material* lovrMaterialInit(Material* material);
#define lovrMaterialCreate() lovrMaterialInit(lovrAlloc(Material))
void lovrMaterialDestroy(void* ref);
void lovrMaterialBind(Material* material, struct Shader* shader);
f32 lovrMaterialGetScalar(Material* material, MaterialScalar scalarType);
void lovrMaterialSetScalar(Material* material, MaterialScalar scalarType, f32 value);
Color lovrMaterialGetColor(Material* material, MaterialColor colorType);
void lovrMaterialSetColor(Material* material, MaterialColor colorType, Color color);
struct Texture* lovrMaterialGetTexture(Material* material, MaterialTexture textureType);
void lovrMaterialSetTexture(Material* material, MaterialTexture textureType, struct Texture* texture);
void lovrMaterialGetTransform(Material* material, f32* ox, f32* oy, f32* sx, f32* sy, f32* angle);
void lovrMaterialSetTransform(Material* material, f32 ox, f32 oy, f32 sx, f32 sy, f32 angle);
