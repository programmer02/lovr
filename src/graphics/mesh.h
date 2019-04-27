#include "util.h"
#include "types.h"
#include "graphics/opengl.h"
#include "lib/gpu.h"
#include "lib/map/map.h"

#pragma once

#define MAX_ATTRIBUTES 16
#define MAX_ATTRIBUTE_NAME_LENGTH 32

struct Buffer;
struct Material;

typedef struct {
  struct Buffer* buffer;
  u32 offset;
  bit stride : 8;
  bit divisor : 8;
  bit type : 3; // AttributeType
  bit components : 3;
  bit normalized : 1;
  bit integer : 1;
  bit disabled : 1;
} MeshAttribute;

typedef struct Mesh {
  Ref ref;
  DrawMode mode;
  char attributeNames[MAX_ATTRIBUTES][MAX_ATTRIBUTE_NAME_LENGTH];
  MeshAttribute attributes[MAX_ATTRIBUTES];
  u8 locations[MAX_ATTRIBUTES];
  u16 enabledLocations;
  u16 divisors[MAX_ATTRIBUTES];
  map_t(u32) attributeMap;
  u32 attributeCount;
  struct Buffer* vertexBuffer;
  struct Buffer* indexBuffer;
  u32 vertexCount;
  u32 indexCount;
  usize indexSize;
  usize indexOffset;
  u32 drawStart;
  u32 drawCount;
  struct Material* material;
  GPU_MESH_FIELDS
} Mesh;

Mesh* lovrMeshInit(Mesh* mesh, DrawMode mode, struct Buffer* vertexBuffer, u32 vertexCount);
#define lovrMeshCreate(...) lovrMeshInit(lovrAlloc(Mesh), __VA_ARGS__)
void lovrMeshDestroy(void* ref);
struct Buffer* lovrMeshGetVertexBuffer(Mesh* mesh);
struct Buffer* lovrMeshGetIndexBuffer(Mesh* mesh);
void lovrMeshSetIndexBuffer(Mesh* mesh, struct Buffer* buffer, u32 indexCount, usize indexSize, usize offset);
u32 lovrMeshGetVertexCount(Mesh* mesh);
u32 lovrMeshGetIndexCount(Mesh* mesh);
usize lovrMeshGetIndexSize(Mesh* mesh);
void lovrMeshAttachAttribute(Mesh* mesh, const char* name, MeshAttribute* attribute);
void lovrMeshDetachAttribute(Mesh* mesh, const char* name);
const MeshAttribute* lovrMeshGetAttribute(Mesh* mesh, const char* name);
bool lovrMeshIsAttributeEnabled(Mesh* mesh, const char* name);
void lovrMeshSetAttributeEnabled(Mesh* mesh, const char* name, bool enabled);
DrawMode lovrMeshGetDrawMode(Mesh* mesh);
void lovrMeshSetDrawMode(Mesh* mesh, DrawMode mode);
void lovrMeshGetDrawRange(Mesh* mesh, u32* start, u32* count);
void lovrMeshSetDrawRange(Mesh* mesh, u32 start, u32 count);
struct Material* lovrMeshGetMaterial(Mesh* mesh);
void lovrMeshSetMaterial(Mesh* mesh, struct Material* material);
