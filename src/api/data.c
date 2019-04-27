#include "api.h"
#include "api/data.h"
#include "data/audioStream.h"
#include "data/modelData.h"
#include "data/rasterizer.h"
#include "data/soundData.h"
#include "data/textureData.h"
#include "lib/err.h"

static int l_lovrDataNewBlob(lua_State* L) {
  usize size;
  u8* data = NULL;
  int type = lua_type(L, 1);
  if (type == LUA_TNUMBER) {
    size = (usize) luax_checku32(L, 1);
    data = calloc(1, size);
    lovrAssert(data, "Out of memory");
  } else if (type == LUA_TSTRING) {
    const char* str = luaL_checklstring(L, 1, &size);
    data = malloc(size + 1);
    lovrAssert(data, "Out of memory");
    memcpy(data, str, size);
    data[size] = '\0';
  } else {
    Blob* blob = luax_checktype(L, 1, Blob);
    size = blob->size;
    data = malloc(size);
    lovrAssert(data, "Out of memory");
    memcpy(data, blob->data, size);
  }
  const char* name = luaL_optstring(L, 2, "");
  Blob* blob = lovrBlobCreate(data, size, name);
  luax_pushobject(L, blob);
  lovrRelease(Blob, blob);
  return 1;
}

static int l_lovrDataNewAudioStream(lua_State* L) {
  Blob* blob = luax_readblob(L, 1, "AudioStream");
  u32 bufferSize = luax_optu32(L, 2, 4096);
  AudioStream* stream = lovrAudioStreamCreate(blob, bufferSize);
  luax_pushobject(L, stream);
  lovrRelease(Blob, blob);
  lovrRelease(AudioStream, stream);
  return 1;
}

static int l_lovrDataNewModelData(lua_State* L) {
  Blob* blob = luax_readblob(L, 1, "Model");
  ModelData* modelData = lovrModelDataCreate(blob);
  luax_pushobject(L, modelData);
  lovrRelease(Blob, blob);
  lovrRelease(ModelData, modelData);
  return 1;
}

static int l_lovrDataNewRasterizer(lua_State* L) {
  Blob* blob = NULL;
  f32 size;

  if (lua_type(L, 1) == LUA_TNUMBER || lua_isnoneornil(L, 1)) {
    size = luax_optf32(L, 1, 32.f);
  } else {
    blob = luax_readblob(L, 1, "Font");
    size = luax_optf32(L, 2, 32.f);
  }

  Rasterizer* rasterizer = lovrRasterizerCreate(blob, size);
  luax_pushobject(L, rasterizer);
  lovrRelease(Blob, blob);
  lovrRelease(Rasterizer, rasterizer);
  return 1;
}

static int l_lovrDataNewSoundData(lua_State* L) {
  if (lua_type(L, 1) == LUA_TNUMBER) {
    usize samples = luax_checku32(L, 1);
    u32 sampleRate = luax_optu32(L, 2, 44100);
    u32 bitDepth = luax_optu32(L, 3, 16);
    u32 channelCount = luax_optu32(L, 4, 2);
    SoundData* soundData = lovrSoundDataCreate(samples, sampleRate, bitDepth, channelCount);
    luax_pushobject(L, soundData);
    lovrRelease(SoundData, soundData);
    return 1;
  }

  AudioStream* audioStream = luax_totype(L, 1, AudioStream);
  if (audioStream) {
    SoundData* soundData = lovrSoundDataCreateFromAudioStream(audioStream);
    luax_pushobject(L, soundData);
    lovrRelease(SoundData, soundData);
    return 1;
  }

  Blob* blob = luax_readblob(L, 1, "SoundData");
  SoundData* soundData = lovrSoundDataCreateFromBlob(blob);
  luax_pushobject(L, soundData);
  lovrRelease(Blob, blob);
  lovrRelease(SoundData, soundData);
  return 1;
}

static int l_lovrDataNewTextureData(lua_State* L) {
  TextureData* textureData = NULL;
  if (lua_type(L, 1) == LUA_TNUMBER) {
    u32 width = luax_checku32(L, 1);
    u32 height = luax_checku32(L, 2);
    TextureFormat format = luaL_checkoption(L, 3, "rgba", TextureFormats);
    textureData = lovrTextureDataCreate(width, height, 0x0, format);
  } else {
    Blob* blob = luax_readblob(L, 1, "Texture");
    bool flip = lua_isnoneornil(L, 2) ? true : lua_toboolean(L, 2);
    textureData = lovrTextureDataCreateFromBlob(blob, flip);
    lovrRelease(Blob, blob);
  }

  luax_pushobject(L, textureData);
  lovrRelease(TextureData, textureData);
  return 1;
}

static const luaL_Reg lovrData[] = {
  { "newBlob", l_lovrDataNewBlob },
  { "newAudioStream", l_lovrDataNewAudioStream },
  { "newModelData", l_lovrDataNewModelData },
  { "newRasterizer", l_lovrDataNewRasterizer },
  { "newSoundData", l_lovrDataNewSoundData },
  { "newTextureData", l_lovrDataNewTextureData },
  { NULL, NULL }
};

int luaopen_lovr_data(lua_State* L) {
  lua_newtable(L);
  luaL_register(L, NULL, lovrData);
  luax_registertype(L, Blob);
  luax_registertype(L, AudioStream);
  luax_registertype(L, ModelData);
  luax_registertype(L, Rasterizer);
  luax_extendtype(L, Blob, SoundData);
  luax_extendtype(L, Blob, TextureData);
  return 1;
}
