#include "util.h"
#include "types.h"
#include "lib/vec/vec.h"
#include "lib/map/map.h"
#include <ode/ode.h>

#pragma once

#define MAX_CONTACTS 4
#define MAX_TAGS 16
#define NO_TAG -1

typedef enum {
  SHAPE_SPHERE,
  SHAPE_BOX,
  SHAPE_CAPSULE,
  SHAPE_CYLINDER
} ShapeType;

typedef enum {
  JOINT_BALL,
  JOINT_DISTANCE,
  JOINT_HINGE,
  JOINT_SLIDER
} JointType;

typedef struct Collider Collider;

typedef struct {
  Ref ref;
  dWorldID id;
  dSpaceID space;
  dJointGroupID contactGroup;
  vec_void_t overlaps;
  map_int_t tags;
  u16 masks[MAX_TAGS];
  Collider* head;
} World;

struct Collider {
  Ref ref;
  dBodyID body;
  World* world;
  Collider* prev;
  Collider* next;
  void* userdata;
  int tag;
  vec_void_t shapes;
  vec_void_t joints;
  f32 friction;
  f32 restitution;
};

typedef struct {
  Ref ref;
  ShapeType type;
  dGeomID id;
  Collider* collider;
  void* userdata;
} Shape;

typedef Shape SphereShape;
typedef Shape BoxShape;
typedef Shape CapsuleShape;
typedef Shape CylinderShape;

typedef struct {
  Ref ref;
  JointType type;
  dJointID id;
  void* userdata;
} Joint;

typedef Joint BallJoint;
typedef Joint DistanceJoint;
typedef Joint HingeJoint;
typedef Joint SliderJoint;

typedef void (*CollisionResolver)(World* world, void* userdata);
typedef void (*RaycastCallback)(Shape* shape, f32 x, f32 y, f32 z, f32 nx, f32 ny, f32 nz, void* userdata);

typedef struct {
  RaycastCallback callback;
  void* userdata;
} RaycastData;

bool lovrPhysicsInit(void);
void lovrPhysicsDestroy(void);

World* lovrWorldInit(World* world, f32 xg, f32 yg, f32 zg, bool allowSleep, const char** tags, u32 tagCount);
#define lovrWorldCreate(...) lovrWorldInit(lovrAlloc(World), __VA_ARGS__)
void lovrWorldDestroy(void* ref);
void lovrWorldDestroyData(World* world);
void lovrWorldUpdate(World* world, f32 dt, CollisionResolver resolver, void* userdata);
void lovrWorldComputeOverlaps(World* world);
bool lovrWorldGetNextOverlap(World* world, Shape** a, Shape** b);
bool lovrWorldCollide(World* world, Shape* a, Shape* b, f32 friction, f32 restitution);
void lovrWorldGetGravity(World* world, f32* x, f32* y, f32* z);
void lovrWorldSetGravity(World* world, f32 x, f32 y, f32 z);
void lovrWorldGetLinearDamping(World* world, f32* damping, f32* threshold);
void lovrWorldSetLinearDamping(World* world, f32 damping, f32 threshold);
void lovrWorldGetAngularDamping(World* world, f32* damping, f32* threshold);
void lovrWorldSetAngularDamping(World* world, f32 damping, f32 threshold);
bool lovrWorldIsSleepingAllowed(World* world);
void lovrWorldSetSleepingAllowed(World* world, bool allowed);
void lovrWorldRaycast(World* world, f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2, RaycastCallback callback, void* userdata);
const char* lovrWorldGetTagName(World* world, int tag);
bool lovrWorldDisableCollisionBetween(World* world, const char* tag1, const char* tag2);
bool lovrWorldEnableCollisionBetween(World* world, const char* tag1, const char* tag2);
bool lovrWorldIsCollisionEnabledBetween(World* world, const char* tag1, const char* tag);

Collider* lovrColliderInit(Collider* collider, World* world, f32 x, f32 y, f32 z);
#define lovrColliderCreate(...) lovrColliderInit(lovrAlloc(Collider), __VA_ARGS__)
void lovrColliderDestroy(void* ref);
void lovrColliderDestroyData(Collider* collider);
World* lovrColliderGetWorld(Collider* collider);
void lovrColliderAddShape(Collider* collider, Shape* shape);
void lovrColliderRemoveShape(Collider* collider, Shape* shape);
vec_void_t* lovrColliderGetShapes(Collider* collider);
vec_void_t* lovrColliderGetJoints(Collider* collider);
void* lovrColliderGetUserData(Collider* collider);
void lovrColliderSetUserData(Collider* collider, void* data);
const char* lovrColliderGetTag(Collider* collider);
bool lovrColliderSetTag(Collider* collider, const char* tag);
f32 lovrColliderGetFriction(Collider* collider);
void lovrColliderSetFriction(Collider* collider, f32 friction);
f32 lovrColliderGetRestitution(Collider* collider);
void lovrColliderSetRestitution(Collider* collider, f32 restitution);
bool lovrColliderIsKinematic(Collider* collider);
void lovrColliderSetKinematic(Collider* collider, bool kinematic);
bool lovrColliderIsGravityIgnored(Collider* collider);
void lovrColliderSetGravityIgnored(Collider* collider, bool ignored);
bool lovrColliderIsSleepingAllowed(Collider* collider);
void lovrColliderSetSleepingAllowed(Collider* collider, bool allowed);
bool lovrColliderIsAwake(Collider* collider);
void lovrColliderSetAwake(Collider* collider, bool awake);
f32 lovrColliderGetMass(Collider* collider);
void lovrColliderSetMass(Collider* collider, f32 mass);
void lovrColliderGetMassData(Collider* collider, f32* cx, f32* cy, f32* cz, f32* mass, f32 inertia[6]);
void lovrColliderSetMassData(Collider* collider, f32 cx, f32 cy, f32 cz, f32 mass, f32 inertia[6]);
void lovrColliderGetPosition(Collider* collider, f32* x, f32* y, f32* z);
void lovrColliderSetPosition(Collider* collider, f32 x, f32 y, f32 z);
void lovrColliderGetOrientation(Collider* collider, f32* angle, f32* x, f32* y, f32* z);
void lovrColliderSetOrientation(Collider* collider, f32 angle, f32 x, f32 y, f32 z);
void lovrColliderGetLinearVelocity(Collider* collider, f32* x, f32* y, f32* z);
void lovrColliderSetLinearVelocity(Collider* collider, f32 x, f32 y, f32 z);
void lovrColliderGetAngularVelocity(Collider* collider, f32* x, f32* y, f32* z);
void lovrColliderSetAngularVelocity(Collider* collider, f32 x, f32 y, f32 z);
void lovrColliderGetLinearDamping(Collider* collider, f32* damping, f32* threshold);
void lovrColliderSetLinearDamping(Collider* collider, f32 damping, f32 threshold);
void lovrColliderGetAngularDamping(Collider* collider, f32* damping, f32* threshold);
void lovrColliderSetAngularDamping(Collider* collider, f32 damping, f32 threshold);
void lovrColliderApplyForce(Collider* collider, f32 x, f32 y, f32 z);
void lovrColliderApplyForceAtPosition(Collider* collider, f32 x, f32 y, f32 z, f32 cx, f32 cy, f32 cz);
void lovrColliderApplyTorque(Collider* collider, f32 x, f32 y, f32 z);
void lovrColliderGetLocalCenter(Collider* collider, f32* x, f32* y, f32* z);
void lovrColliderGetLocalPoint(Collider* collider, f32 wx, f32 wy, f32 wz, f32* x, f32* y, f32* z);
void lovrColliderGetWorldPoint(Collider* collider, f32 x, f32 y, f32 z, f32* wx, f32* wy, f32* wz);
void lovrColliderGetLocalVector(Collider* collider, f32 wx, f32 wy, f32 wz, f32* x, f32* y, f32* z);
void lovrColliderGetWorldVector(Collider* collider, f32 x, f32 y, f32 z, f32* wx, f32* wy, f32* wz);
void lovrColliderGetLinearVelocityFromLocalPoint(Collider* collider, f32 x, f32 y, f32 z, f32* vx, f32* vy, f32* vz);
void lovrColliderGetLinearVelocityFromWorldPoint(Collider* collider, f32 wx, f32 wy, f32 wz, f32* vx, f32* vy, f32* vz);
void lovrColliderGetAABB(Collider* collider, f32 aabb[6]);

void lovrShapeDestroy(void* ref);
void lovrShapeDestroyData(Shape* shape);
ShapeType lovrShapeGetType(Shape* shape);
Collider* lovrShapeGetCollider(Shape* shape);
bool lovrShapeIsEnabled(Shape* shape);
void lovrShapeSetEnabled(Shape* shape, bool enabled);
void* lovrShapeGetUserData(Shape* shape);
void lovrShapeSetUserData(Shape* shape, void* data);
void lovrShapeGetPosition(Shape* shape, f32* x, f32* y, f32* z);
void lovrShapeSetPosition(Shape* shape, f32 x, f32 y, f32 z);
void lovrShapeGetOrientation(Shape* shape, f32* angle, f32* x, f32* y, f32* z);
void lovrShapeSetOrientation(Shape* shape, f32 angle, f32 x, f32 y, f32 z);
void lovrShapeGetMass(Shape* shape, f32 density, f32* cx, f32* cy, f32* cz, f32* mass, f32 inertia[6]);
void lovrShapeGetAABB(Shape* shape, f32 aabb[6]);

SphereShape* lovrSphereShapeInit(SphereShape* sphere, f32 radius);
#define lovrSphereShapeCreate(...) lovrSphereShapeInit(lovrAlloc(SphereShape), __VA_ARGS__)
#define lovrSphereShapeDestroy lovrShapeDestroy
f32 lovrSphereShapeGetRadius(SphereShape* sphere);
void lovrSphereShapeSetRadius(SphereShape* sphere, f32 radius);

BoxShape* lovrBoxShapeInit(BoxShape* box, f32 x, f32 y, f32 z);
#define lovrBoxShapeCreate(...) lovrBoxShapeInit(lovrAlloc(BoxShape), __VA_ARGS__)
#define lovrBoxShapeDestroy lovrShapeDestroy
void lovrBoxShapeGetDimensions(BoxShape* box, f32* x, f32* y, f32* z);
void lovrBoxShapeSetDimensions(BoxShape* box, f32 x, f32 y, f32 z);

CapsuleShape* lovrCapsuleShapeInit(CapsuleShape* capsule, f32 radius, f32 length);
#define lovrCapsuleShapeCreate(...) lovrCapsuleShapeInit(lovrAlloc(CapsuleShape), __VA_ARGS__)
#define lovrCapsuleShapeDestroy lovrShapeDestroy
f32 lovrCapsuleShapeGetRadius(CapsuleShape* capsule);
void lovrCapsuleShapeSetRadius(CapsuleShape* capsule, f32 radius);
f32 lovrCapsuleShapeGetLength(CapsuleShape* capsule);
void lovrCapsuleShapeSetLength(CapsuleShape* capsule, f32 length);

CylinderShape* lovrCylinderShapeInit(CylinderShape* cylinder, f32 radius, f32 length);
#define lovrCylinderShapeCreate(...) lovrCylinderShapeInit(lovrAlloc(CylinderShape), __VA_ARGS__)
#define lovrCylinderShapeDestroy lovrShapeDestroy
f32 lovrCylinderShapeGetRadius(CylinderShape* cylinder);
void lovrCylinderShapeSetRadius(CylinderShape* cylinder, f32 radius);
f32 lovrCylinderShapeGetLength(CylinderShape* cylinder);
void lovrCylinderShapeSetLength(CylinderShape* cylinder, f32 length);

void lovrJointDestroy(void* ref);
void lovrJointDestroyData(Joint* joint);
JointType lovrJointGetType(Joint* joint);
void lovrJointGetColliders(Joint* joint, Collider** a, Collider** b);
void* lovrJointGetUserData(Joint* joint);
void lovrJointSetUserData(Joint* joint, void* data);

BallJoint* lovrBallJointInit(BallJoint* joint, Collider* a, Collider* b, f32 x, f32 y, f32 z);
#define lovrBallJointCreate(...) lovrBallJointInit(lovrAlloc(BallJoint), __VA_ARGS__)
#define lovrBallJointDestroy lovrJointDestroy
void lovrBallJointGetAnchors(BallJoint* joint, f32* x1, f32* y1, f32* z1, f32* x2, f32* y2, f32* z2);
void lovrBallJointSetAnchor(BallJoint* joint, f32 x, f32 y, f32 z);

DistanceJoint* lovrDistanceJointInit(DistanceJoint* joint, Collider* a, Collider* b, f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2);
#define lovrDistanceJointCreate(...) lovrDistanceJointInit(lovrAlloc(DistanceJoint), __VA_ARGS__)
#define lovrDistanceJointDestroy lovrJointDestroy
void lovrDistanceJointGetAnchors(DistanceJoint* joint, f32* x1, f32* y1, f32* z1, f32* x2, f32* y2, f32* z2);
void lovrDistanceJointSetAnchors(DistanceJoint* joint, f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2);
f32 lovrDistanceJointGetDistance(DistanceJoint* joint);
void lovrDistanceJointSetDistance(DistanceJoint* joint, f32 distance);

HingeJoint* lovrHingeJointInit(HingeJoint* joint, Collider* a, Collider* b, f32 x, f32 y, f32 z, f32 ax, f32 ay, f32 az);
#define lovrHingeJointCreate(...) lovrHingeJointInit(lovrAlloc(HingeJoint), __VA_ARGS__)
#define lovrHingeJointDestroy lovrJointDestroy
void lovrHingeJointGetAnchors(HingeJoint* joint, f32* x1, f32* y1, f32* z1, f32* x2, f32* y2, f32* z2);
void lovrHingeJointSetAnchor(HingeJoint* joint, f32 x, f32 y, f32 z);
void lovrHingeJointGetAxis(HingeJoint* joint, f32* x, f32* y, f32* z);
void lovrHingeJointSetAxis(HingeJoint* joint, f32 x, f32 y, f32 z);
f32 lovrHingeJointGetAngle(HingeJoint* joint);
f32 lovrHingeJointGetLowerLimit(HingeJoint* joint);
void lovrHingeJointSetLowerLimit(HingeJoint* joint, f32 limit);
f32 lovrHingeJointGetUpperLimit(HingeJoint* joint);
void lovrHingeJointSetUpperLimit(HingeJoint* joint, f32 limit);

SliderJoint* lovrSliderJointInit(SliderJoint* joint, Collider* a, Collider* b, f32 ax, f32 ay, f32 az);
#define lovrSliderJointCreate(...) lovrSliderJointInit(lovrAlloc(SliderJoint), __VA_ARGS__)
#define lovrSliderJointDestroy lovrJointDestroy
void lovrSliderJointGetAxis(SliderJoint* joint, f32* x, f32* y, f32* z);
void lovrSliderJointSetAxis(SliderJoint* joint, f32 x, f32 y, f32 z);
f32 lovrSliderJointGetPosition(SliderJoint* joint);
f32 lovrSliderJointGetLowerLimit(SliderJoint* joint);
void lovrSliderJointSetLowerLimit(SliderJoint* joint, f32 limit);
f32 lovrSliderJointGetUpperLimit(SliderJoint* joint);
void lovrSliderJointSetUpperLimit(SliderJoint* joint, f32 limit);
