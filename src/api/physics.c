#include "api.h"
#include "physics/physics.h"

const char* ShapeTypes[] = {
  [SHAPE_SPHERE] = "sphere",
  [SHAPE_BOX] = "box",
  [SHAPE_CAPSULE] = "capsule",
  [SHAPE_CYLINDER] = "cylinder",
  NULL
};

const char* JointTypes[] = {
  [JOINT_BALL] = "ball",
  [JOINT_DISTANCE] = "distance",
  [JOINT_HINGE] = "hinge",
  [JOINT_SLIDER] = "slider",
  NULL
};

static int l_lovrPhysicsNewWorld(lua_State* L) {
  f32 xg = luax_optfloat(L, 1, 0.f);
  f32 yg = luax_optfloat(L, 2, -9.81f);
  f32 zg = luax_optfloat(L, 3, 0.f);
  bool allowSleep = lua_gettop(L) < 4 || lua_toboolean(L, 4);
  const char* tags[16];
  u32 tagCount;
  if (lua_type(L, 5) == LUA_TTABLE) {
    tagCount = lua_objlen(L, 5);
    for (u32 i = 0; i < tagCount; i++) {
      lua_rawgeti(L, -1, i + 1);
      if (lua_isstring(L, -1)) {
        tags[i] = lua_tostring(L, -1);
      } else {
        return luaL_error(L, "World tags must be a table of strings");
      }
      lua_pop(L, 1);
    }
  } else {
    tagCount = 0;
  }
  World* world = lovrWorldCreate(xg, yg, zg, allowSleep, tags, tagCount);
  luax_pushobject(L, world);
  lovrRelease(World, world);
  return 1;
}

static int l_lovrPhysicsNewBallJoint(lua_State* L) {
  Collider* a = luax_checktype(L, 1, Collider);
  Collider* b = luax_checktype(L, 2, Collider);
  f32 x = luax_checkfloat(L, 3);
  f32 y = luax_checkfloat(L, 4);
  f32 z = luax_checkfloat(L, 5);
  BallJoint* joint = lovrBallJointCreate(a, b, x, y, z);
  luax_pushobject(L, joint);
  lovrRelease(Joint, joint);
  return 1;
}

static int l_lovrPhysicsNewBoxShape(lua_State* L) {
  f32 x = luax_optfloat(L, 1, 1.f);
  f32 y = luax_optfloat(L, 2, x);
  f32 z = luax_optfloat(L, 3, x);
  BoxShape* box = lovrBoxShapeCreate(x, y, z);
  luax_pushobject(L, box);
  lovrRelease(Shape, box);
  return 1;
}

static int l_lovrPhysicsNewCapsuleShape(lua_State* L) {
  f32 radius = luax_optfloat(L, 1, 1.f);
  f32 length = luax_optfloat(L, 2, 1.f);
  CapsuleShape* capsule = lovrCapsuleShapeCreate(radius, length);
  luax_pushobject(L, capsule);
  lovrRelease(Shape, capsule);
  return 1;
}

static int l_lovrPhysicsNewCylinderShape(lua_State* L) {
  f32 radius = luax_optfloat(L, 1, 1.f);
  f32 length = luax_optfloat(L, 2, 1.f);
  CylinderShape* cylinder = lovrCylinderShapeCreate(radius, length);
  luax_pushobject(L, cylinder);
  lovrRelease(Shape, cylinder);
  return 1;
}

static int l_lovrPhysicsNewDistanceJoint(lua_State* L) {
  Collider* a = luax_checktype(L, 1, Collider);
  Collider* b = luax_checktype(L, 2, Collider);
  f32 x1 = luax_checkfloat(L, 3);
  f32 y1 = luax_checkfloat(L, 4);
  f32 z1 = luax_checkfloat(L, 5);
  f32 x2 = luax_checkfloat(L, 6);
  f32 y2 = luax_checkfloat(L, 7);
  f32 z2 = luax_checkfloat(L, 8);
  DistanceJoint* joint = lovrDistanceJointCreate(a, b, x1, y1, z1, x2, y2, z2);
  luax_pushobject(L, joint);
  lovrRelease(Joint, joint);
  return 1;
}

static int l_lovrPhysicsNewHingeJoint(lua_State* L) {
  Collider* a = luax_checktype(L, 1, Collider);
  Collider* b = luax_checktype(L, 2, Collider);
  f32 x = luax_checkfloat(L, 3);
  f32 y = luax_checkfloat(L, 4);
  f32 z = luax_checkfloat(L, 5);
  f32 ax = luax_checkfloat(L, 6);
  f32 ay = luax_checkfloat(L, 7);
  f32 az = luax_checkfloat(L, 8);
  HingeJoint* joint = lovrHingeJointCreate(a, b, x, y, z, ax, ay, az);
  luax_pushobject(L, joint);
  lovrRelease(Joint, joint);
  return 1;
}

static int l_lovrPhysicsNewSliderJoint(lua_State* L) {
  Collider* a = luax_checktype(L, 1, Collider);
  Collider* b = luax_checktype(L, 2, Collider);
  f32 ax = luax_checkfloat(L, 3);
  f32 ay = luax_checkfloat(L, 4);
  f32 az = luax_checkfloat(L, 5);
  SliderJoint* joint = lovrSliderJointCreate(a, b, ax, ay, az);
  luax_pushobject(L, joint);
  lovrRelease(Joint, joint);
  return 1;
}

static int l_lovrPhysicsNewSphereShape(lua_State* L) {
  f32 radius = luax_optfloat(L, 1, 1.f);
  SphereShape* sphere = lovrSphereShapeCreate(radius);
  luax_pushobject(L, sphere);
  lovrRelease(Shape, sphere);
  return 1;
}

static const luaL_Reg lovrPhysics[] = {
  { "newWorld", l_lovrPhysicsNewWorld },
  { "newBallJoint", l_lovrPhysicsNewBallJoint },
  { "newBoxShape", l_lovrPhysicsNewBoxShape },
  { "newCapsuleShape", l_lovrPhysicsNewCapsuleShape },
  { "newCylinderShape", l_lovrPhysicsNewCylinderShape },
  { "newDistanceJoint", l_lovrPhysicsNewDistanceJoint },
  { "newHingeJoint", l_lovrPhysicsNewHingeJoint },
  { "newSliderJoint", l_lovrPhysicsNewSliderJoint },
  { "newSphereShape", l_lovrPhysicsNewSphereShape },
  { NULL, NULL }
};

int luaopen_lovr_physics(lua_State* L) {
  lua_newtable(L);
  luaL_register(L, NULL, lovrPhysics);
  luax_registertype(L, World);
  luax_registertype(L, Collider);
  luax_extendtype(L, Joint, BallJoint);
  luax_extendtype(L, Joint, DistanceJoint);
  luax_extendtype(L, Joint, HingeJoint);
  luax_extendtype(L, Joint, SliderJoint);
  luax_extendtype(L, Shape, SphereShape);
  luax_extendtype(L, Shape, BoxShape);
  luax_extendtype(L, Shape, CapsuleShape);
  luax_extendtype(L, Shape, CylinderShape);
  if (lovrPhysicsInit()) {
    luax_atexit(L, lovrPhysicsDestroy);
  }
  return 1;
}
