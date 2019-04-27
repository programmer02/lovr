#include "api.h"
#include "api/data.h"
#include "api/math.h"
#include "graphics/buffer.h"
#include "graphics/graphics.h"
#include "graphics/material.h"
#include "graphics/mesh.h"
#include "lib/err.h"
#include <limits.h>

static int l_lovrMeshAttachAttributes(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  Mesh* other = luax_checktype(L, 2, Mesh);
  u32 instanceDivisor = luax_optu32(L, 3, 0);
  if (lua_isnoneornil(L, 4)) {
    for (u32 i = 0; i < other->attributeCount; i++) {
      MeshAttribute attachment = other->attributes[i];
      if (attachment.buffer != other->vertexBuffer) {
        break;
      }
      attachment.divisor = instanceDivisor;
      lovrMeshAttachAttribute(mesh, other->attributeNames[i], &attachment);
    }
  } else if (lua_istable(L, 4)) {
    usize length = lua_objlen(L, 4);
    for (usize i = 0; i < length; i++) {
      lua_rawgeti(L, 4, i + 1);
      const char* name = lua_tostring(L, -1);
      const MeshAttribute* attribute = lovrMeshGetAttribute(other, name);
      lovrAssert(attribute, "Tried to attach non-existent attribute %s", name);
      MeshAttribute attachment = *attribute;
      attachment.divisor = instanceDivisor;
      lovrMeshAttachAttribute(mesh, name, &attachment);
      lua_pop(L, 1);
    }
  } else {
    int top = lua_gettop(L);
    for (int i = 4; i <= top; i++) {
      const char* name = lua_tostring(L, i);
      const MeshAttribute* attribute = lovrMeshGetAttribute(other, name);
      lovrAssert(attribute, "Tried to attach non-existent attribute %s", name);
      MeshAttribute attachment = *attribute;
      attachment.divisor = instanceDivisor;
      lovrMeshAttachAttribute(mesh, name, &attachment);
    }
  }

  return 0;
}

static int l_lovrMeshDetachAttributes(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  if (lua_isuserdata(L, 2)) {
    Mesh* other = luax_checktype(L, 2, Mesh);
    for (u32 i = 0; i < other->attributeCount; i++) {
      const MeshAttribute* attachment = &other->attributes[i];
      if (attachment->buffer != other->vertexBuffer) {
        break;
      }
      lovrMeshDetachAttribute(mesh, other->attributeNames[i]);
    }
  } else if (lua_istable(L, 2)) {
    usize length = lua_objlen(L, 2);
    for (usize i = 0; i < length; i++) {
      lua_rawgeti(L, 2, i + 1);
      lovrMeshDetachAttribute(mesh, lua_tostring(L, -1));
      lua_pop(L, 1);
    }
  } else {
    int top = lua_gettop(L);
    for (int i = 2; i <= top; i++) {
      lovrMeshDetachAttribute(mesh, lua_tostring(L, i));
    }
  }
  return 0;
}

static int l_lovrMeshDraw(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  f32 transform[16];
  int index = luax_readmat4(L, 2, transform, 1);
  u32 instances = luax_optu32(L, index, 1);
  u32 vertexCount = lovrMeshGetVertexCount(mesh);
  u32 indexCount = lovrMeshGetIndexCount(mesh);
  u32 baseCount = indexCount > 0 ? indexCount : vertexCount;
  u32 rangeStart, rangeCount;
  lovrMeshGetDrawRange(mesh, &rangeStart, &rangeCount);
  lovrGraphicsBatch(&(BatchRequest) {
    .type = BATCH_MESH,
    .params.mesh = {
      .object = mesh,
      .mode = lovrMeshGetDrawMode(mesh),
      .rangeStart = rangeStart,
      .rangeCount = rangeCount ? rangeCount : baseCount,
      .instances = instances
    },
    .drawMode = lovrMeshGetDrawMode(mesh),
    .transform = transform,
    .material = lovrMeshGetMaterial(mesh)
  });
  return 0;
}

static int l_lovrMeshGetDrawMode(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  lua_pushstring(L, DrawModes[lovrMeshGetDrawMode(mesh)]);
  return 1;
}

static int l_lovrMeshSetDrawMode(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  DrawMode mode = luaL_checkoption(L, 2, NULL, DrawModes);
  lovrMeshSetDrawMode(mesh, mode);
  return 0;
}

static int l_lovrMeshGetVertexFormat(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  lua_createtable(L, mesh->attributeCount, 0);
  for (u32 i = 0; i < mesh->attributeCount; i++) {
    const MeshAttribute* attribute = &mesh->attributes[i];
    if (attribute->buffer != mesh->vertexBuffer) {
      break;
    }
    lua_createtable(L, 3, 0);
    lua_pushstring(L, mesh->attributeNames[i]);
    lua_rawseti(L, -2, 1);
    lua_pushstring(L, AttributeTypes[attribute->type]);
    lua_rawseti(L, -2, 2);
    lua_pushinteger(L, attribute->components);
    lua_rawseti(L, -2, 3);
    lua_rawseti(L, -2, i + 1);
  }
  return 1;
}

static int l_lovrMeshGetVertexCount(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  lua_pushinteger(L, lovrMeshGetVertexCount(mesh));
  return 1;
}

static int l_lovrMeshGetVertex(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  u32 index = luax_checku32(L, 2) - 1;
  lovrAssert(index >= 1 && index <= lovrMeshGetVertexCount(mesh), "Invalid mesh vertex index: %d", index);
  index--;

  if (!mesh->vertexBuffer || mesh->attributeCount == 0 || mesh->attributes[0].buffer != mesh->vertexBuffer) {
    lovrThrow("Mesh does not have a vertex buffer");
  }

  lovrAssert(lovrBufferIsReadable(mesh->vertexBuffer), "Mesh:getVertex can only be used if the Mesh was created with the readable flag");
  AttributeData data = { .raw = lovrBufferMap(mesh->vertexBuffer, index * mesh->attributes[0].stride) };

  u32 components = 0;
  for (u32 i = 0; i < mesh->attributeCount; i++) {
    const MeshAttribute* attribute = &mesh->attributes[i];
    if (attribute->buffer != mesh->vertexBuffer) {
      break;
    }
    for (u32 j = 0; j < attribute->components; j++, components++) {
      switch (attribute->type) {
        case I8: lua_pushinteger(L, *data.bytes++); break;
        case U8: lua_pushinteger(L, *data.ubytes++); break;
        case I16: lua_pushinteger(L, *data.shorts++); break;
        case U16: lua_pushinteger(L, *data.ushorts++); break;
        case I32: lua_pushinteger(L, *data.ints++); break;
        case U32: lua_pushinteger(L, *data.uints++); break;
        case F32: lua_pushnumber(L, *data.floats++); break;
      }
    }
  }
  return components;
}

static int l_lovrMeshSetVertex(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  u32 index = luax_checku32(L, 2);
  lovrAssert(index >= 1 && index <= lovrMeshGetVertexCount(mesh), "Invalid mesh vertex index: %d", index);
  index--;
  bool table = lua_istable(L, 3);

  if (!mesh->vertexBuffer || mesh->attributeCount == 0 || mesh->attributes[0].buffer != mesh->vertexBuffer) {
    lovrThrow("Mesh does not have a vertex buffer");
  }

  usize stride = mesh->attributes[0].stride;
  AttributeData data = { .raw = lovrBufferMap(mesh->vertexBuffer, index * stride) };
  u32 component = 0;
  for (u32 i = 0; i < mesh->attributeCount; i++) {
    const MeshAttribute* attribute = &mesh->attributes[i];
    if (attribute->buffer != mesh->vertexBuffer) {
      break;
    }

    for (u32 j = 0; j < attribute->components; j++) {
      int k = 3 + j;
      if (table) {
        lua_rawgeti(L, 3, ++component);
        k = -1;
      }

      switch (attribute->type) {
        case I8: *data.bytes++ = luaL_optinteger(L, k, 0); break;
        case U8: *data.ubytes++ = luaL_optinteger(L, k, 0); break;
        case I16: *data.shorts++ = luaL_optinteger(L, k, 0); break;
        case U16: *data.ubytes++ = luaL_optinteger(L, k, 0); break;
        case I32: *data.ints++ = luaL_optinteger(L, k, 0); break;
        case U32: *data.uints++ = luaL_optinteger(L, k, 0); break;
        case F32: *data.floats++ = luax_optf32(L, k, 0.); break;
      }

      if (table) {
        lua_pop(L, 1);
      }
    }
  }
  lovrBufferMarkRange(mesh->vertexBuffer, index * stride, (index + 1) * stride);
  return 0;
}

static int l_lovrMeshGetVertexAttribute(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  u32 vertexIndex = luax_checku32(L, 2);
  u32 attributeIndex = luax_checku32(L, 3);
  Buffer* buffer = lovrMeshGetVertexBuffer(mesh);
  lovrAssert(lovrBufferIsReadable(buffer), "Mesh:getVertex can only be used if the Mesh was created with the readable flag");
  lovrAssert(vertexIndex >= 1 && vertexIndex <= lovrMeshGetVertexCount(mesh), "Invalid mesh vertex: %d", vertexIndex);
  lovrAssert(attributeIndex >= 1 && attributeIndex <= mesh->attributeCount, "Invalid mesh attribute: %d", attributeIndex);
  lovrAssert(mesh->attributes[attributeIndex - 1].buffer == mesh->vertexBuffer, "Invalid mesh attribute: %d", attributeIndex);
  MeshAttribute* attribute = &mesh->attributes[attributeIndex - 1];
  AttributeData data = { .raw = lovrBufferMap(buffer, (vertexIndex - 1) * attribute->stride + attribute->offset) };
  for (u32 i = 0; i < attribute->components; i++) {
    switch (attribute->type) {
      case I8: lua_pushinteger(L, *data.bytes++); break;
      case U8: lua_pushinteger(L, *data.ubytes++); break;
      case I16: lua_pushinteger(L, *data.shorts++); break;
      case U16: lua_pushinteger(L, *data.ushorts++); break;
      case I32: lua_pushinteger(L, *data.ints++); break;
      case U32: lua_pushinteger(L, *data.uints++); break;
      case F32: lua_pushnumber(L, *data.floats++); break;
    }
  }
  return attribute->components;
}

static int l_lovrMeshSetVertexAttribute(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  u32 vertexIndex = luax_checku32(L, 2);
  u32 attributeIndex = luax_checku32(L, 3);
  bool table = lua_istable(L, 4);
  lovrAssert(vertexIndex >= 1 && vertexIndex <= lovrMeshGetVertexCount(mesh), "Invalid mesh vertex: %d", vertexIndex);
  lovrAssert(attributeIndex >= 1 && attributeIndex <= mesh->attributeCount, "Invalid mesh attribute: %d", attributeIndex);
  lovrAssert(mesh->attributes[attributeIndex].buffer == mesh->vertexBuffer, "Invalid mesh attribute: %d", attributeIndex);
  vertexIndex--, attributeIndex--;
  MeshAttribute* attribute = &mesh->attributes[attributeIndex];
  AttributeData data = { .raw = lovrBufferMap(mesh->vertexBuffer, vertexIndex * attribute->stride + attribute->offset) };
  for (u32 i = 0; i < attribute->components; i++) {
    int index = 4 + i;
    if (table) {
      lua_rawgeti(L, 4, i + 1);
      index = -1;
    }

    switch (attribute->type) {
      case I8: *data.bytes++ = luaL_optinteger(L, index, 0); break;
      case U8: *data.ubytes++ = luaL_optinteger(L, index, 0); break;
      case I16: *data.shorts++ = luaL_optinteger(L, index, 0); break;
      case U16: *data.ushorts++ = luaL_optinteger(L, index, 0); break;
      case I32: *data.ints++ = luaL_optinteger(L, index, 0); break;
      case U32: *data.uints++ = luaL_optinteger(L, index, 0); break;
      case F32: *data.floats++ = luax_optf32(L, index, 0.); break;
    }

    if (table) {
      lua_pop(L, 1);
    }
  }
  lovrBufferMarkRange(mesh->vertexBuffer, vertexIndex * attribute->stride, (vertexIndex + 1) * attribute->stride);
  return 0;
}

static int l_lovrMeshSetVertices(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  u32 capacity = lovrMeshGetVertexCount(mesh);
  luaL_checktype(L, 2, LUA_TTABLE);
  u32 sourceSize = luax_len(L, 2);
  u32 start = luax_optu32(L, 3, 1) - 1;
  u32 count = luax_optu32(L, 4, sourceSize);
  lovrAssert(start + count <= capacity, "Overflow in Mesh:setVertices: Mesh can only hold %d vertices", capacity);
  lovrAssert(count <= sourceSize, "Cannot set %d vertices on Mesh: source only has %d vertices", count, sourceSize);

  if (!mesh->vertexBuffer || mesh->attributeCount == 0 || mesh->attributes[0].buffer != mesh->vertexBuffer) {
    lovrThrow("Mesh does not have a vertex buffer");
  }

  usize stride = mesh->attributes[0].stride;
  AttributeData data = { .raw = lovrBufferMap(mesh->vertexBuffer, start * stride) };

  for (u32 i = 0; i < count; i++) {
    lua_rawgeti(L, 2, i + 1);
    luaL_checktype(L, -1, LUA_TTABLE);
    u32 component = 0;
    for (u32 j = 0; j < mesh->attributeCount; j++) {
      const MeshAttribute* attribute = &mesh->attributes[j];
      if (attribute->buffer != mesh->vertexBuffer) {
        break;
      }

      for (u32 k = 0; k < attribute->components; k++) {
        lua_rawgeti(L, -1, ++component);

        switch (attribute->type) {
          case I8: *data.bytes++ = luaL_optinteger(L, -1, 0); break;
          case U8: *data.ubytes++ = luaL_optinteger(L, -1, 0); break;
          case I16: *data.shorts++ = luaL_optinteger(L, -1, 0); break;
          case U16: *data.ushorts++ = luaL_optinteger(L, -1, 0); break;
          case I32: *data.ints++ = luaL_optinteger(L, -1, 0); break;
          case U32: *data.uints++ = luaL_optinteger(L, -1, 0); break;
          case F32: *data.floats++ = luax_optf32(L, -1, 0.); break;
        }

        lua_pop(L, 1);
      }
    }
    lua_pop(L, 1);
  }

  lovrBufferMarkRange(mesh->vertexBuffer, start * stride, (start + count) * stride);
  return 0;
}

static int l_lovrMeshGetVertexMap(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  Buffer* buffer = lovrMeshGetIndexBuffer(mesh);
  u32 count = lovrMeshGetIndexCount(mesh);
  usize size = lovrMeshGetIndexSize(mesh);

  if (!buffer || count == 0 || size == 0) {
    lua_pushnil(L);
    return 1;
  }

  lovrAssert(lovrBufferIsReadable(buffer), "Mesh:getVertexMap can only be used if the Mesh was created with the readable flag");
  union { void* raw; u16* shorts; u32* ints; } indices = { .raw = lovrBufferMap(buffer, 0) };

  if (lua_istable(L, 2)) {
    lua_settop(L, 2);
  } else if (lua_isuserdata(L, 2)) {
    Blob* blob = luax_checktype(L, 2, Blob);
    lovrAssert(size * count <= blob->size, "Mesh vertex map is %zu bytes, but Blob can only hold %zu", size * count, blob->size);
    memcpy(blob->data, indices.raw, size * count);
    return 0;
  } else {
    lua_settop(L, 1);
    lua_createtable(L, count, 0);
  }

  for (u32 i = 0; i < count; i++) {
    u32 index = size == sizeof(u32) ? indices.ints[i] : indices.shorts[i];
    lua_pushinteger(L, index + 1);
    lua_rawseti(L, 2, i + 1);
  }

  return 1;
}

static int l_lovrMeshSetVertexMap(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);

  if (lua_isnoneornil(L, 2)) {
    lovrMeshSetIndexBuffer(mesh, NULL, 0, 0, 0);
    return 0;
  }

  if (lua_type(L, 2) == LUA_TUSERDATA) {
    Blob* blob = luax_checktype(L, 2, Blob);
    usize size = luax_optu32(L, 3, 4);
    lovrAssert(size == 2 || size == 4, "Size of Mesh indices should be 2 bytes or 4 bytes");
    lovrAssert(blob->size / size < UINT32_MAX, "Too many Mesh indices");
    u32 count = (u32) (blob->size / size);
    Buffer* indexBuffer = lovrMeshGetIndexBuffer(mesh);
    if (!indexBuffer || count * size > lovrBufferGetSize(indexBuffer)) {
      Buffer* vertexBuffer = lovrMeshGetVertexBuffer(mesh);
      BufferUsage usage = vertexBuffer ? lovrBufferGetUsage(vertexBuffer) : USAGE_DYNAMIC;
      bool readable = vertexBuffer ? lovrBufferIsReadable(vertexBuffer) : false;
      indexBuffer = lovrBufferInit(lovrNew(Buffer), blob->size, blob->data, BUFFER_INDEX, usage, readable);
      lovrMeshSetIndexBuffer(mesh, indexBuffer, count, size, 0);
    } else {
      void* indices = lovrBufferMap(indexBuffer, 0);
      memcpy(indices, blob->data, blob->size);
      lovrBufferMarkRange(indexBuffer, 0, blob->size);
    }
  } else {
    luaL_checktype(L, 2, LUA_TTABLE);
    u32 count = luax_len(L, 2);
    u32 vertexCount = lovrMeshGetVertexCount(mesh);
    usize size = vertexCount > USHRT_MAX ? sizeof(u32) : sizeof(u16);

    Buffer* indexBuffer = lovrMeshGetIndexBuffer(mesh);
    if (!indexBuffer || count * size > lovrBufferGetSize(indexBuffer)) {
      Buffer* vertexBuffer = lovrMeshGetVertexBuffer(mesh);
      BufferUsage usage = vertexBuffer ? lovrBufferGetUsage(vertexBuffer) : USAGE_DYNAMIC;
      bool readable = vertexBuffer ? lovrBufferIsReadable(vertexBuffer) : false;
      indexBuffer = lovrBufferInit(lovrNew(Buffer), count * size, NULL, BUFFER_INDEX, usage, readable);
    }

    union { void* raw; u16* shorts; u32* ints; } indices = { .raw = lovrBufferMap(indexBuffer, 0) };

    for (u32 i = 0; i < count; i++) {
      lua_rawgeti(L, 2, i + 1);
      if (!lua_isnumber(L, -1)) {
        return luaL_error(L, "Mesh vertex map index #%d must be numeric", i);
      }

      u32 index = lua_tointeger(L, -1);
      if (index > vertexCount || index < 1) {
        return luaL_error(L, "Invalid vertex map value: %d", index);
      }

      if (size == sizeof(u16)) {
        indices.shorts[i] = index - 1;
      } else {
        indices.ints[i] = index - 1;
      }

      lua_pop(L, 1);
    }

    lovrMeshSetIndexBuffer(mesh, indexBuffer, count, size, 0);
    lovrBufferMarkRange(indexBuffer, 0, count * size);
  }

  return 0;
}

static int l_lovrMeshIsAttributeEnabled(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  const char* attribute = luaL_checkstring(L, 2);
  lua_pushboolean(L, lovrMeshIsAttributeEnabled(mesh, attribute));
  return 1;
}

static int l_lovrMeshSetAttributeEnabled(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  const char* attribute = luaL_checkstring(L, 2);
  bool enabled = lua_toboolean(L, 3);
  lovrMeshSetAttributeEnabled(mesh, attribute, enabled);
  return 0;
}

static int l_lovrMeshGetDrawRange(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  u32 start, count;
  lovrMeshGetDrawRange(mesh, &start, &count);

  if (count == 0) {
    lua_pushnil(L);
    return 1;
  }

  lua_pushinteger(L, start + 1);
  lua_pushinteger(L, count);
  return 2;
}

static int l_lovrMeshSetDrawRange(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  if (lua_isnoneornil(L, 2)) {
    lovrMeshSetDrawRange(mesh, 0, 0);
    return 0;
  }

  u32 rangeStart = luax_checku32(L, 2) - 1;
  u32 rangeCount = luax_checku32(L, 3);
  lovrMeshSetDrawRange(mesh, rangeStart, rangeCount);
  return 0;
}

static int l_lovrMeshGetMaterial(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  Material* material = lovrMeshGetMaterial(mesh);
  luax_pushobject(L, material);
  return 1;
}

static int l_lovrMeshSetMaterial(lua_State* L) {
  Mesh* mesh = luax_checktype(L, 1, Mesh);
  if (lua_isnoneornil(L, 2)) {
    lovrMeshSetMaterial(mesh, NULL);
  } else {
    Material* material = luax_checktype(L, 2, Material);
    lovrMeshSetMaterial(mesh, material);
  }
  return 0;
}

const luaL_Reg lovrMesh[] = {
  { "attachAttributes", l_lovrMeshAttachAttributes },
  { "detachAttributes", l_lovrMeshDetachAttributes },
  { "draw", l_lovrMeshDraw },
  { "getVertexFormat", l_lovrMeshGetVertexFormat },
  { "getVertexCount", l_lovrMeshGetVertexCount },
  { "getVertex", l_lovrMeshGetVertex },
  { "setVertex", l_lovrMeshSetVertex },
  { "getVertexAttribute", l_lovrMeshGetVertexAttribute },
  { "setVertexAttribute", l_lovrMeshSetVertexAttribute },
  { "setVertices", l_lovrMeshSetVertices },
  { "getVertexMap", l_lovrMeshGetVertexMap },
  { "setVertexMap", l_lovrMeshSetVertexMap },
  { "isAttributeEnabled", l_lovrMeshIsAttributeEnabled },
  { "setAttributeEnabled", l_lovrMeshSetAttributeEnabled },
  { "getDrawMode", l_lovrMeshGetDrawMode },
  { "setDrawMode", l_lovrMeshSetDrawMode },
  { "getDrawRange", l_lovrMeshGetDrawRange },
  { "setDrawRange", l_lovrMeshSetDrawRange },
  { "getMaterial", l_lovrMeshGetMaterial },
  { "setMaterial", l_lovrMeshSetMaterial },
  { NULL, NULL }
};
