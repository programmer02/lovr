#include "api.h"
#include "graphics/animator.h"

static u32 luax_checkanimation(lua_State* L, int index, Animator* animator) {
  switch (lua_type(L, index)) {
    case LUA_TNUMBER: {
      u32 i = luax_checku32(L, index);
      lovrAssert(i >= 1 && i <= lovrAnimatorGetAnimationCount(animator), "Invalid animation '%d'", i);
      return i - 1;
    }
    case LUA_TSTRING: {
      const char* name = lua_tostring(L, index);
      u32* i = lovrAnimatorGetAnimationIndex(animator, name);
      lovrAssert(i, "Unknown animation '%s'", name);
      return *i;
    }
    default:
      luaL_typerror(L, index, "number or string");
      return ~0u;
  }
}

static int l_lovrAnimatorReset(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  lovrAnimatorReset(animator);
  return 0;
}

static int l_lovrAnimatorUpdate(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  f32 dt = luax_checkfloat(L, 2);
  lovrAnimatorUpdate(animator, dt);
  return 0;
}

static int l_lovrAnimatorGetAnimationCount(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  lua_pushnumber(L, lovrAnimatorGetAnimationCount(animator));
  return 1;
}

static int l_lovrAnimatorGetAnimationNames(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animationCount = lovrAnimatorGetAnimationCount(animator);

  if (lua_istable(L, 2)) {
    lua_settop(L, 2);
  } else {
    lua_settop(L, 1);
    lua_createtable(L, animationCount, 0);
  }

  for (u32 i = 0; i < animationCount; i++) {
    lua_pushstring(L, lovrAnimatorGetAnimationName(animator, i));
    lua_rawseti(L, -2, i + 1);
  }

  return 1;
}

static int l_lovrAnimatorPlay(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  lovrAnimatorPlay(animator, animation);
  return 0;
}

static int l_lovrAnimatorStop(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  lovrAnimatorStop(animator, animation);
  return 0;
}

static int l_lovrAnimatorPause(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  lovrAnimatorPause(animator, animation);
  return 0;
}

static int l_lovrAnimatorResume(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  lovrAnimatorResume(animator, animation);
  return 0;
}

static int l_lovrAnimatorSeek(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  f32 time = luax_checkfloat(L, 3);
  lovrAnimatorSeek(animator, animation, time);
  return 0;
}

static int l_lovrAnimatorTell(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  f32 time = lovrAnimatorTell(animator, animation);
  lua_pushnumber(L, time);
  return 1;
}

static int l_lovrAnimatorGetAlpha(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  f32 alpha = lovrAnimatorGetAlpha(animator, animation);
  lua_pushnumber(L, alpha);
  return 1;
}

static int l_lovrAnimatorSetAlpha(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  f32 alpha = luax_checkfloat(L, 3);
  lovrAnimatorSetAlpha(animator, animation, alpha);
  return 0;
}

static int l_lovrAnimatorGetDuration(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  f32 duration = lovrAnimatorGetDuration(animator, animation);
  lua_pushnumber(L, duration);
  return 1;
}

static int l_lovrAnimatorIsPlaying(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  bool playing = lovrAnimatorIsPlaying(animator, animation);
  lua_pushboolean(L, playing);
  return 1;
}

static int l_lovrAnimatorIsLooping(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  bool looping = lovrAnimatorIsLooping(animator, animation);
  lua_pushboolean(L, looping);
  return 1;
}

static int l_lovrAnimatorSetLooping(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  bool looping = lua_toboolean(L, 3);
  lovrAnimatorSetLooping(animator, animation, looping);
  return 0;
}

static int l_lovrAnimatorGetPriority(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  i32 priority = lovrAnimatorGetPriority(animator, animation);
  lua_pushinteger(L, priority);
  return 1;
}

static int l_lovrAnimatorSetPriority(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  u32 animation = luax_checkanimation(L, 2, animator);
  i32 priority = luaL_checkinteger(L, 3);
  lovrAnimatorSetPriority(animator, animation, priority);
  return 0;
}

static int l_lovrAnimatorGetSpeed(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  if (lua_isnoneornil(L, 2)) {
    f32 speed = lovrAnimatorGetSpeed(animator, ~0u);
    lua_pushnumber(L, speed);
  } else {
    u32 animation = luax_checkanimation(L, 2, animator);
    f32 speed = lovrAnimatorGetSpeed(animator, animation);
    lua_pushnumber(L, speed);
  }
  return 1;
}

static int l_lovrAnimatorSetSpeed(lua_State* L) {
  Animator* animator = luax_checktype(L, 1, Animator);
  if (lua_isnoneornil(L, 2)) {
    f32 speed = luax_checkfloat(L, 2);
    lovrAnimatorSetSpeed(animator, ~0u, speed);
  } else {
    i32 animation = luax_checkanimation(L, 2, animator);
    f32 speed = luax_checkfloat(L, 3);
    lovrAnimatorSetSpeed(animator, animation, speed);
  }
  return 0;
}

const luaL_Reg lovrAnimator[] = {
  { "reset", l_lovrAnimatorReset },
  { "update", l_lovrAnimatorUpdate },
  { "getAnimationCount", l_lovrAnimatorGetAnimationCount },
  { "getAnimationNames", l_lovrAnimatorGetAnimationNames },
  { "play", l_lovrAnimatorPlay },
  { "stop", l_lovrAnimatorStop },
  { "pause", l_lovrAnimatorPause },
  { "resume", l_lovrAnimatorResume },
  { "seek", l_lovrAnimatorSeek },
  { "tell", l_lovrAnimatorTell },
  { "getAlpha", l_lovrAnimatorGetAlpha },
  { "setAlpha", l_lovrAnimatorSetAlpha },
  { "getDuration", l_lovrAnimatorGetDuration },
  { "isPlaying", l_lovrAnimatorIsPlaying },
  { "isLooping", l_lovrAnimatorIsLooping },
  { "setLooping", l_lovrAnimatorSetLooping },
  { "getPriority", l_lovrAnimatorGetPriority },
  { "setPriority", l_lovrAnimatorSetPriority },
  { "getSpeed", l_lovrAnimatorGetSpeed },
  { "setSpeed", l_lovrAnimatorSetSpeed },
  { NULL, NULL }
};
