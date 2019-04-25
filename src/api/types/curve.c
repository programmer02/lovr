#include "api.h"
#include "api/math.h"
#include "math/curve.h"
#include "lib/err.h"

static int l_lovrCurveEvaluate(lua_State* L) {
  Curve* curve = luax_checktype(L, 1, Curve);
  f32 t = luax_checkfloat(L, 2);
  f32 point[3];
  lovrCurveEvaluate(curve, t, point);
  lua_pushnumber(L, point[0]);
  lua_pushnumber(L, point[1]);
  lua_pushnumber(L, point[2]);
  return 3;
}

static int l_lovrCurveGetTangent(lua_State* L) {
  Curve* curve = luax_checktype(L, 1, Curve);
  f32 t = luax_checkfloat(L, 2);
  f32 point[3];
  lovrCurveGetTangent(curve, t, point);
  lua_pushnumber(L, point[0]);
  lua_pushnumber(L, point[1]);
  lua_pushnumber(L, point[2]);
  return 3;
}

static int l_lovrCurveRender(lua_State* L) {
  Curve* curve = luax_checktype(L, 1, Curve);
  usize n = luax_optu32(L, 2, 32);
  f32 t1 = luax_optfloat(L, 3, 0.);
  f32 t2 = luax_optfloat(L, 4, 1.);
  f32* points = malloc(3 * n * sizeof(f32));
  lovrAssert(points, "Out of memory");
  lovrCurveRender(curve, t1, t2, points, n);
  lua_createtable(L, n, 0);
  for (usize i = 0; i < 3 * n; i++) {
    lua_pushnumber(L, points[i]);
    lua_rawseti(L, -2, i + 1);
  }
  free(points);
  return 1;
}

static int l_lovrCurveSlice(lua_State* L) {
  Curve* curve = luax_checktype(L, 1, Curve);
  f32 t1 = luax_checkfloat(L, 2);
  f32 t2 = luax_checkfloat(L, 3);
  Curve* subcurve = lovrCurveSlice(curve, t1, t2);
  luax_pushobject(L, subcurve);
  return 1;
}

static int l_lovrCurveGetPointCount(lua_State* L) {
  Curve* curve = luax_checktype(L, 1, Curve);
  lua_pushinteger(L, lovrCurveGetPointCount(curve));
  return 1;
}

static int l_lovrCurveGetPoint(lua_State* L) {
  Curve* curve = luax_checktype(L, 1, Curve);
  usize index = luax_checku32(L, 2);
  lovrAssert(index >= 1 && index <= lovrCurveGetPointCount(curve), "Invalid Curve point index: %d", index);
  f32 point[3];
  lovrCurveGetPoint(curve, index - 1, point);
  lua_pushnumber(L, point[0]);
  lua_pushnumber(L, point[1]);
  lua_pushnumber(L, point[2]);
  return 3;
}

static int l_lovrCurveSetPoint(lua_State* L) {
  Curve* curve = luax_checktype(L, 1, Curve);
  usize index = luax_checku32(L, 2);
  lovrAssert(index >= 1 && index <= lovrCurveGetPointCount(curve), "Invalid Curve point index: %d", index);
  f32 point[3];
  luax_readvec3(L, 3, point, NULL);
  lovrCurveSetPoint(curve, index - 1, point);
  return 0;
}

static int l_lovrCurveAddPoint(lua_State* L) {
  Curve* curve = luax_checktype(L, 1, Curve);
  f32 point[3];
  int i = luax_readvec3(L, 2, point, NULL);
  usize index = lua_isnoneornil(L, i) ? lovrCurveGetPointCount(curve) + 1 : luax_checku32(L, i);
  lovrAssert(index >= 1 && index <= lovrCurveGetPointCount(curve) + 1, "Invalid Curve point index: %d", index);
  lovrCurveAddPoint(curve, point, index - 1);
  return 0;
}

static int l_lovrCurveRemovePoint(lua_State* L) {
  Curve* curve = luax_checktype(L, 1, Curve);
  usize index = luax_checku32(L, 2);
  lovrAssert(index >= 1 && index < lovrCurveGetPointCount(curve), "Invalid Curve point index: %d", index);
  lovrCurveRemovePoint(curve, index - 1);
  return 0;
}

const luaL_Reg lovrCurve[] = {
  { "evaluate", l_lovrCurveEvaluate },
  { "getTangent", l_lovrCurveGetTangent },
  { "render", l_lovrCurveRender },
  { "slice", l_lovrCurveSlice },
  { "getPointCount", l_lovrCurveGetPointCount },
  { "getPoint", l_lovrCurveGetPoint },
  { "setPoint", l_lovrCurveSetPoint },
  { "addPoint", l_lovrCurveAddPoint },
  { "removePoint", l_lovrCurveRemovePoint },
  { NULL, NULL }
};
