#include "physics.h"
#include "lib/err.h"
#include "lib/maf.h"

static void defaultNearCallback(void* data, dGeomID a, dGeomID b) {
  lovrWorldCollide((World*) data, dGeomGetData(a), dGeomGetData(b), -1, -1);
}

static void customNearCallback(void* data, dGeomID shapeA, dGeomID shapeB) {
  World* world = data;
  vec_push(&world->overlaps, dGeomGetData(shapeA));
  vec_push(&world->overlaps, dGeomGetData(shapeB));
}

static void raycastCallback(void* data, dGeomID a, dGeomID b) {
  RaycastCallback callback = ((RaycastData*) data)->callback;
  void* userdata = ((RaycastData*) data)->userdata;
  Shape* shape = dGeomGetData(b);

  if (!shape) {
    return;
  }

  dContact contact;
  if (dCollide(a, b, MAX_CONTACTS, &contact.geom, sizeof(dContact))) {
    dContactGeom g = contact.geom;
    callback(shape, g.pos[0], g.pos[1], g.pos[2], g.normal[0], g.normal[1], g.normal[2], userdata);
  }
}

static bool initialized = false;

bool lovrPhysicsInit() {
  if (initialized) return false;
  dInitODE();
  return initialized = true;
}

void lovrPhysicsDestroy() {
  if (!initialized) return;
  dCloseODE();
  initialized = false;
}

World* lovrWorldInit(World* world, f32 xg, f32 yg, f32 zg, bool allowSleep, const char** tags, u32 tagCount) {
  world->id = dWorldCreate();
  world->space = dHashSpaceCreate(0);
  dHashSpaceSetLevels(world->space, -4, 8);
  world->contactGroup = dJointGroupCreate(0);
  vec_init(&world->overlaps);
  lovrWorldSetGravity(world, xg, yg, zg);
  lovrWorldSetSleepingAllowed(world, allowSleep);
  map_init(&world->tags);
  for (u32 i = 0; i < tagCount; i++) {
    map_set(&world->tags, tags[i], i);
  }

  for (u32 i = 0; i < MAX_TAGS; i++) {
    world->masks[i] = 0xffff;
  }

  return world;
}

void lovrWorldDestroy(void* ref) {
  World* world = ref;
  lovrWorldDestroyData(world);
  vec_deinit(&world->overlaps);
  map_deinit(&world->tags);
}

void lovrWorldDestroyData(World* world) {
  while (world->head) {
    Collider* next = world->head->next;
    lovrColliderDestroyData(world->head);
    world->head = next;
  }

  if (world->contactGroup) {
    dJointGroupDestroy(world->contactGroup);
    world->contactGroup = NULL;
  }

  if (world->space) {
    dSpaceDestroy(world->space);
    world->space = NULL;
  }

  if (world->id) {
    dWorldDestroy(world->id);
    world->id = NULL;
  }
}

void lovrWorldUpdate(World* world, f32 dt, CollisionResolver resolver, void* userdata) {
  if (resolver) {
    resolver(world, userdata);
  } else {
    dSpaceCollide(world->space, world, defaultNearCallback);
  }

  if (dt > 0.f) {
    dWorldQuickStep(world->id, dt);
  }

  dJointGroupEmpty(world->contactGroup);
}

void lovrWorldComputeOverlaps(World* world) {
  vec_clear(&world->overlaps);
  dSpaceCollide(world->space, world, customNearCallback);
}

bool lovrWorldGetNextOverlap(World* world, Shape** a, Shape** b) {
  if (world->overlaps.length == 0) {
    *a = *b = NULL;
    return false;
  }

  *a = vec_pop(&world->overlaps);
  *b = vec_pop(&world->overlaps);
  return true;
}

bool lovrWorldCollide(World* world, Shape* a, Shape* b, f32 friction, f32 restitution) {
  if (!a || !b) {
    return false;
  }

  Collider* colliderA = a->collider;
  Collider* colliderB = b->collider;
  u32 tag1 = colliderA->tag;
  u32 tag2 = colliderB->tag;

  if (tag1 != NO_TAG && tag2 != NO_TAG && !((world->masks[tag1] & (1 << tag2)) && (world->masks[tag2] & (1 << tag1)))) {
    return false;
  }

  if (friction < 0.f) {
    friction = sqrtf(colliderA->friction * colliderB->friction);
  }

  if (restitution < 0.f) {
    restitution = MAX(colliderA->restitution, colliderB->restitution);
  }

  dContact contacts[MAX_CONTACTS];
  for (u32 i = 0; i < MAX_CONTACTS; i++) {
    contacts[i].surface.mode = 0;
    contacts[i].surface.mu = friction;
    contacts[i].surface.bounce = restitution;
    contacts[i].surface.mu = dInfinity;

    if (restitution > 0) {
      contacts[i].surface.mode |= dContactBounce;
    }
  }

  int contactCount = dCollide(a->id, b->id, MAX_CONTACTS, &contacts[0].geom, sizeof(dContact));

  for (int i = 0; i < contactCount; i++) {
    dJointID joint = dJointCreateContact(world->id, world->contactGroup, &contacts[i]);
    dJointAttach(joint, colliderA->body, colliderB->body);
  }

  return contactCount > 0;
}

void lovrWorldGetGravity(World* world, f32* x, f32* y, f32* z) {
  dReal gravity[3];
  dWorldGetGravity(world->id, gravity);
  *x = gravity[0];
  *y = gravity[1];
  *z = gravity[2];
}

void lovrWorldSetGravity(World* world, f32 x, f32 y, f32 z) {
  dWorldSetGravity(world->id, x, y, z);
}

void lovrWorldGetLinearDamping(World* world, f32* damping, f32* threshold) {
  *damping = dWorldGetLinearDamping(world->id);
  *threshold = dWorldGetLinearDampingThreshold(world->id);
}

void lovrWorldSetLinearDamping(World* world, f32 damping, f32 threshold) {
  dWorldSetLinearDamping(world->id, damping);
  dWorldSetLinearDampingThreshold(world->id, threshold);
}

void lovrWorldGetAngularDamping(World* world, f32* damping, f32* threshold) {
  *damping = dWorldGetAngularDamping(world->id);
  *threshold = dWorldGetAngularDampingThreshold(world->id);
}

void lovrWorldSetAngularDamping(World* world, f32 damping, f32 threshold) {
  dWorldSetAngularDamping(world->id, damping);
  dWorldSetAngularDampingThreshold(world->id, threshold);
}

bool lovrWorldIsSleepingAllowed(World* world) {
  return dWorldGetAutoDisableFlag(world->id);
}

void lovrWorldSetSleepingAllowed(World* world, bool allowed) {
  dWorldSetAutoDisableFlag(world->id, allowed);
}

void lovrWorldRaycast(World* world, f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2, RaycastCallback callback, void* userdata) {
  RaycastData data = { .callback = callback, .userdata = userdata };
  f32 dx = x2 - x1;
  f32 dy = y2 - y1;
  f32 dz = z2 - z1;
  f32 length = sqrtf(dx * dx + dy * dy + dz * dz);
  dGeomID ray = dCreateRay(world->space, length);
  dGeomRaySet(ray, x1, y1, z1, dx, dy, dz);
  dSpaceCollide2(ray, (dGeomID) world->space, &data, raycastCallback);
  dGeomDestroy(ray);
}

const char* lovrWorldGetTagName(World* world, u32 tag) {
  if (tag == NO_TAG) {
    return NULL;
  }

  const char* key;
  map_iter_t iter = map_iter(&world->tags);
  while ((key = map_next(&world->tags, &iter))) {
    if (*map_get(&world->tags, key) == tag) {
      return key;
    }
  }

  return NULL;
}

bool lovrWorldDisableCollisionBetween(World* world, const char* tag1, const char* tag2) {
  u32* index1 = map_get(&world->tags, tag1);
  u32* index2 = map_get(&world->tags, tag2);
  if (!index1 || !index2) {
    return false;
  }

  world->masks[*index1] &= ~(1 << *index2);
  world->masks[*index2] &= ~(1 << *index1);
  return true;
}

bool lovrWorldEnableCollisionBetween(World* world, const char* tag1, const char* tag2) {
  u32* index1 = map_get(&world->tags, tag1);
  u32* index2 = map_get(&world->tags, tag2);
  if (!index1 || !index2) {
    return false;
  }

  world->masks[*index1] |= (1 << *index2);
  world->masks[*index2] |= (1 << *index1);
  return true;
}

bool lovrWorldIsCollisionEnabledBetween(World* world, const char* tag1, const char* tag2) {
  u32* index1 = map_get(&world->tags, tag1);
  u32* index2 = map_get(&world->tags, tag2);
  if (!index1 || !index2) {
    return false;
  }

  return (world->masks[*index1] & (1 << *index2)) && (world->masks[*index2] & (1 << *index1));
}

Collider* lovrColliderInit(Collider* collider, World* world, f32 x, f32 y, f32 z) {
  collider->body = dBodyCreate(world->id);
  collider->world = world;
  collider->friction = 0.f;
  collider->restitution = 0.f;
  collider->tag = NO_TAG;
  dBodySetData(collider->body, collider);
  vec_init(&collider->shapes);
  vec_init(&collider->joints);

  lovrColliderSetPosition(collider, x, y, z);

  // Adjust the world's collider list
  if (!collider->world->head) {
    collider->world->head = collider;
  } else {
    collider->next = collider->world->head;
    collider->next->prev = collider;
    collider->world->head = collider;
  }

  // The world owns a reference to the collider
  lovrRetain(collider);
  return collider;
}

void lovrColliderDestroy(void* ref) {
  Collider* collider = ref;
  lovrColliderDestroyData(collider);
  vec_deinit(&collider->shapes);
  vec_deinit(&collider->joints);
}

void lovrColliderDestroyData(Collider* collider) {
  if (!collider->body) {
    return;
  }

  vec_void_t* shapes = lovrColliderGetShapes(collider);
  Shape* shape; int i;
  vec_foreach(shapes, shape, i) {
    lovrColliderRemoveShape(collider, shape);
  }

  vec_void_t* joints = lovrColliderGetJoints(collider);
  Joint* joint; int j;
  vec_foreach(joints, joint, j) {
    lovrRelease(Joint, joint);
  }

  dBodyDestroy(collider->body);
  collider->body = NULL;

  if (collider->next) collider->next->prev = collider->prev;
  if (collider->prev) collider->prev->next = collider->next;
  if (collider->world->head == collider) collider->world->head = collider->next;
  collider->next = collider->prev = NULL;

  // If the Collider is destroyed, the world lets go of its reference to this Collider
  lovrRelease(Collider, collider);
}

World* lovrColliderGetWorld(Collider* collider) {
  return collider->world;
}

void lovrColliderAddShape(Collider* collider, Shape* shape) {
  lovrRetain(shape);

  if (shape->collider) {
    lovrColliderRemoveShape(shape->collider, shape);
  }

  shape->collider = collider;
  dGeomSetBody(shape->id, collider->body);
  dSpaceID newSpace = collider->world->space;
  dSpaceAdd(newSpace, shape->id);
}

void lovrColliderRemoveShape(Collider* collider, Shape* shape) {
  if (shape->collider == collider) {
    dSpaceRemove(collider->world->space, shape->id);
    dGeomSetBody(shape->id, 0);
    shape->collider = NULL;
    lovrRelease(Shape, shape);
  }
}

vec_void_t* lovrColliderGetShapes(Collider* collider) {
  vec_clear(&collider->shapes);
  for (dGeomID geom = dBodyGetFirstGeom(collider->body); geom; geom = dBodyGetNextGeom(geom)) {
    Shape* shape = dGeomGetData(geom);
    if (shape) {
      vec_push(&collider->shapes, shape);
    }
  }
  return &collider->shapes;
}

vec_void_t* lovrColliderGetJoints(Collider* collider) {
  vec_clear(&collider->joints);
  int jointCount = dBodyGetNumJoints(collider->body);
  for (int i = 0; i < jointCount; i++) {
    Joint* joint = dJointGetData(dBodyGetJoint(collider->body, i));
    if (joint) {
      vec_push(&collider->joints, joint);
    }
  }
  return &collider->joints;
}

void* lovrColliderGetUserData(Collider* collider) {
  return collider->userdata;
}

void lovrColliderSetUserData(Collider* collider, void* data) {
  collider->userdata = data;
}

const char* lovrColliderGetTag(Collider* collider) {
  return lovrWorldGetTagName(collider->world, collider->tag);
}

bool lovrColliderSetTag(Collider* collider, const char* tag) {
  if (tag == NULL) {
    collider->tag = NO_TAG;
    return true;
  }

  u32* index = map_get(&collider->world->tags, tag);

  if (!index) {
    return false;
  }

  collider->tag = *index;
  return true;
}

f32 lovrColliderGetFriction(Collider* collider) {
  return collider->friction;
}

void lovrColliderSetFriction(Collider* collider, f32 friction) {
  collider->friction = friction;
}

f32 lovrColliderGetRestitution(Collider* collider) {
  return collider->restitution;
}

void lovrColliderSetRestitution(Collider* collider, f32 restitution) {
  collider->restitution = restitution;
}

bool lovrColliderIsKinematic(Collider* collider) {
  return dBodyIsKinematic(collider->body);
}

void lovrColliderSetKinematic(Collider* collider, bool kinematic) {
  if (kinematic) {
    dBodySetKinematic(collider->body);
  } else {
    dBodySetDynamic(collider->body);
  }
}

bool lovrColliderIsGravityIgnored(Collider* collider) {
  return !dBodyGetGravityMode(collider->body);
}

void lovrColliderSetGravityIgnored(Collider* collider, bool ignored) {
  dBodySetGravityMode(collider->body, !ignored);
}

bool lovrColliderIsSleepingAllowed(Collider* collider) {
  return dBodyGetAutoDisableFlag(collider->body);
}

void lovrColliderSetSleepingAllowed(Collider* collider, bool allowed) {
  dBodySetAutoDisableFlag(collider->body, allowed);
}

bool lovrColliderIsAwake(Collider* collider) {
  return dBodyIsEnabled(collider->body);
}

void lovrColliderSetAwake(Collider* collider, bool awake) {
  if (awake) {
    dBodyEnable(collider->body);
  } else {
    dBodyDisable(collider->body);
  }
}

f32 lovrColliderGetMass(Collider* collider) {
  dMass m;
  dBodyGetMass(collider->body, &m);
  return m.mass;
}

void lovrColliderSetMass(Collider* collider, f32 mass) {
  dMass m;
  dBodyGetMass(collider->body, &m);
  dMassAdjust(&m, mass);
  dBodySetMass(collider->body, &m);
}

void lovrColliderGetMassData(Collider* collider, f32* cx, f32* cy, f32* cz, f32* mass, f32 inertia[6]) {
  dMass m;
  dBodyGetMass(collider->body, &m);
  *cx = m.c[0];
  *cy = m.c[1];
  *cz = m.c[2];
  *mass = m.mass;

  // Diagonal
  inertia[0] = m.I[0];
  inertia[1] = m.I[5];
  inertia[2] = m.I[10];

  // Lower triangular
  inertia[3] = m.I[4];
  inertia[4] = m.I[8];
  inertia[5] = m.I[9];
}

void lovrColliderSetMassData(Collider* collider, f32 cx, f32 cy, f32 cz, f32 mass, f32 inertia[]) {
  dMass m;
  dBodyGetMass(collider->body, &m);
  dMassSetParameters(&m, mass, cx, cy, cz, inertia[0], inertia[1], inertia[2], inertia[3], inertia[4], inertia[5]);
  dBodySetMass(collider->body, &m);
}

void lovrColliderGetPosition(Collider* collider, f32* x, f32* y, f32* z) {
  const dReal* position = dBodyGetPosition(collider->body);
  *x = position[0];
  *y = position[1];
  *z = position[2];
}

void lovrColliderSetPosition(Collider* collider, f32 x, f32 y, f32 z) {
  dBodySetPosition(collider->body, x, y, z);
}

void lovrColliderGetOrientation(Collider* collider, f32* angle, f32* x, f32* y, f32* z) {
  const dReal* q = dBodyGetQuaternion(collider->body);
  f32 quaternion[4] = { q[1], q[2], q[3], q[0] };
  quat_getAngleAxis(quaternion, angle, x, y, z);
}

void lovrColliderSetOrientation(Collider* collider, f32 angle, f32 x, f32 y, f32 z) {
  f32 quaternion[4];
  quat_fromAngleAxis(quaternion, angle, x, y, z);
  f32 q[4] = { quaternion[3], quaternion[0], quaternion[1], quaternion[2] };
  dBodySetQuaternion(collider->body, q);
}

void lovrColliderGetLinearVelocity(Collider* collider, f32* x, f32* y, f32* z) {
  const dReal* velocity = dBodyGetLinearVel(collider->body);
  *x = velocity[0];
  *y = velocity[1];
  *z = velocity[2];
}

void lovrColliderSetLinearVelocity(Collider* collider, f32 x, f32 y, f32 z) {
  dBodySetLinearVel(collider->body, x, y, z);
}

void lovrColliderGetAngularVelocity(Collider* collider, f32* x, f32* y, f32* z) {
  const dReal* velocity = dBodyGetAngularVel(collider->body);
  *x = velocity[0];
  *y = velocity[1];
  *z = velocity[2];
}

void lovrColliderSetAngularVelocity(Collider* collider, f32 x, f32 y, f32 z) {
  dBodySetAngularVel(collider->body, x, y, z);
}

void lovrColliderGetLinearDamping(Collider* collider, f32* damping, f32* threshold) {
  *damping = dBodyGetLinearDamping(collider->body);
  *threshold = dBodyGetLinearDampingThreshold(collider->body);
}

void lovrColliderSetLinearDamping(Collider* collider, f32 damping, f32 threshold) {
  dBodySetLinearDamping(collider->body, damping);
  dBodySetLinearDampingThreshold(collider->body, threshold);
}

void lovrColliderGetAngularDamping(Collider* collider, f32* damping, f32* threshold) {
  *damping = dBodyGetAngularDamping(collider->body);
  *threshold = dBodyGetAngularDampingThreshold(collider->body);
}

void lovrColliderSetAngularDamping(Collider* collider, f32 damping, f32 threshold) {
  dBodySetAngularDamping(collider->body, damping);
  dBodySetAngularDampingThreshold(collider->body, threshold);
}

void lovrColliderApplyForce(Collider* collider, f32 x, f32 y, f32 z) {
  dBodyAddForce(collider->body, x, y, z);
}

void lovrColliderApplyForceAtPosition(Collider* collider, f32 x, f32 y, f32 z, f32 cx, f32 cy, f32 cz) {
  dBodyAddForceAtPos(collider->body, x, y, z, cx, cy, cz);
}

void lovrColliderApplyTorque(Collider* collider, f32 x, f32 y, f32 z) {
  dBodyAddTorque(collider->body, x, y, z);
}

void lovrColliderGetLocalCenter(Collider* collider, f32* x, f32* y, f32* z) {
  dMass m;
  dBodyGetMass(collider->body, &m);
  *x = m.c[0];
  *y = m.c[1];
  *z = m.c[2];
}

void lovrColliderGetLocalPoint(Collider* collider, f32 wx, f32 wy, f32 wz, f32* x, f32* y, f32* z) {
  dReal local[3];
  dBodyGetPosRelPoint(collider->body, wx, wy, wz, local);
  *x = local[0];
  *y = local[1];
  *z = local[2];
}

void lovrColliderGetWorldPoint(Collider* collider, f32 x, f32 y, f32 z, f32* wx, f32* wy, f32* wz) {
  dReal world[3];
  dBodyGetRelPointPos(collider->body, x, y, z, world);
  *wx = world[0];
  *wy = world[1];
  *wz = world[2];
}

void lovrColliderGetLocalVector(Collider* collider, f32 wx, f32 wy, f32 wz, f32* x, f32* y, f32* z) {
  dReal local[3];
  dBodyVectorFromWorld(collider->body, wx, wy, wz, local);
  *x = local[0];
  *y = local[1];
  *z = local[2];
}

void lovrColliderGetWorldVector(Collider* collider, f32 x, f32 y, f32 z, f32* wx, f32* wy, f32* wz) {
  dReal world[3];
  dBodyVectorToWorld(collider->body, x, y, z, world);
  *wx = world[0];
  *wy = world[1];
  *wz = world[2];
}

void lovrColliderGetLinearVelocityFromLocalPoint(Collider* collider, f32 x, f32 y, f32 z, f32* vx, f32* vy, f32* vz) {
  dReal velocity[3];
  dBodyGetRelPointVel(collider->body, x, y, z, velocity);
  *vx = velocity[0];
  *vy = velocity[1];
  *vz = velocity[2];
}

void lovrColliderGetLinearVelocityFromWorldPoint(Collider* collider, f32 wx, f32 wy, f32 wz, f32* vx, f32* vy, f32* vz) {
  dReal velocity[3];
  dBodyGetPointVel(collider->body, wx, wy, wz, velocity);
  *vx = velocity[0];
  *vy = velocity[1];
  *vz = velocity[2];
}

void lovrColliderGetAABB(Collider* collider, f32 aabb[6]) {
  dGeomID shape = dBodyGetFirstGeom(collider->body);

  if (!shape) {
    memset(aabb, 0, 6 * sizeof(f32));
    return;
  }

  dGeomGetAABB(shape, aabb);

  f32 otherAABB[6];
  while ((shape = dBodyGetNextGeom(shape)) != NULL) {
    dGeomGetAABB(shape, otherAABB);
    aabb[0] = MIN(aabb[0], otherAABB[0]);
    aabb[1] = MAX(aabb[1], otherAABB[1]);
    aabb[2] = MIN(aabb[2], otherAABB[2]);
    aabb[3] = MAX(aabb[3], otherAABB[3]);
    aabb[4] = MIN(aabb[4], otherAABB[4]);
    aabb[5] = MAX(aabb[5], otherAABB[5]);
  }
}

void lovrShapeDestroy(void* ref) {
  Shape* shape = ref;
  lovrShapeDestroyData(shape);
}

void lovrShapeDestroyData(Shape* shape) {
  if (shape->id) {
    dGeomDestroy(shape->id);
    shape->id = NULL;
  }
}

ShapeType lovrShapeGetType(Shape* shape) {
  return shape->type;
}

Collider* lovrShapeGetCollider(Shape* shape) {
  return shape->collider;
}

bool lovrShapeIsEnabled(Shape* shape) {
  return dGeomIsEnabled(shape->id);
}

void lovrShapeSetEnabled(Shape* shape, bool enabled) {
  if (enabled) {
    dGeomEnable(shape->id);
  } else {
    dGeomDisable(shape->id);
  }
}

void* lovrShapeGetUserData(Shape* shape) {
  return shape->userdata;
}

void lovrShapeSetUserData(Shape* shape, void* data) {
  shape->userdata = data;
}

void lovrShapeGetPosition(Shape* shape, f32* x, f32* y, f32* z) {
  const dReal* position = dGeomGetOffsetPosition(shape->id);
  *x = position[0];
  *y = position[1];
  *z = position[2];
}

void lovrShapeSetPosition(Shape* shape, f32 x, f32 y, f32 z) {
  dGeomSetOffsetPosition(shape->id, x, y, z);
}

void lovrShapeGetOrientation(Shape* shape, f32* angle, f32* x, f32* y, f32* z) {
  dReal q[4];
  dGeomGetOffsetQuaternion(shape->id, q);
  f32 quaternion[4] = { q[1], q[2], q[3], q[0] };
  quat_getAngleAxis(quaternion, angle, x, y, z);
}

void lovrShapeSetOrientation(Shape* shape, f32 angle, f32 x, f32 y, f32 z) {
  f32 quaternion[4];
  quat_fromAngleAxis(quaternion, angle, x, y, z);
  f32 q[4] = { quaternion[3], quaternion[0], quaternion[1], quaternion[2] };
  dGeomSetOffsetQuaternion(shape->id, q);
}

void lovrShapeGetMass(Shape* shape, f32 density, f32* cx, f32* cy, f32* cz, f32* mass, f32 inertia[6]) {
  dMass m;
  dMassSetZero(&m);
  switch (shape->type) {
    case SHAPE_SPHERE: {
      dMassSetSphere(&m, density, dGeomSphereGetRadius(shape->id));
      break;
    }

    case SHAPE_BOX: {
      dReal lengths[3];
      dGeomBoxGetLengths(shape->id, lengths);
      dMassSetBox(&m, density, lengths[0], lengths[1], lengths[2]);
      break;
    }

    case SHAPE_CAPSULE: {
      dReal radius, length;
      dGeomCapsuleGetParams(shape->id, &radius, &length);
      dMassSetCapsule(&m, density, 3, radius, length);
      break;
    }

    case SHAPE_CYLINDER: {
      dReal radius, length;
      dGeomCylinderGetParams(shape->id, &radius, &length);
      dMassSetCylinder(&m, density, 3, radius, length);
      break;
    }
  }

  const dReal* position = dGeomGetOffsetPosition(shape->id);
  dMassTranslate(&m, position[0], position[1], position[2]);
  const dReal* rotation = dGeomGetOffsetRotation(shape->id);
  dMassRotate(&m, rotation);

  *cx = m.c[0];
  *cy = m.c[1];
  *cz = m.c[2];
  *mass = m.mass;

  // Diagonal
  inertia[0] = m.I[0];
  inertia[1] = m.I[5];
  inertia[2] = m.I[10];

  // Lower triangular
  inertia[3] = m.I[4];
  inertia[4] = m.I[8];
  inertia[5] = m.I[9];
}

void lovrShapeGetAABB(Shape* shape, f32 aabb[6]) {
  dGeomGetAABB(shape->id, aabb);
}

SphereShape* lovrSphereShapeInit(SphereShape* sphere, f32 radius) {
  sphere->type = SHAPE_SPHERE;
  sphere->id = dCreateSphere(0, radius);
  dGeomSetData(sphere->id, sphere);
  return sphere;
}

f32 lovrSphereShapeGetRadius(SphereShape* sphere) {
  return dGeomSphereGetRadius(sphere->id);
}

void lovrSphereShapeSetRadius(SphereShape* sphere, f32 radius) {
  dGeomSphereSetRadius(sphere->id, radius);
}

BoxShape* lovrBoxShapeInit(BoxShape* box, f32 x, f32 y, f32 z) {
  box->type = SHAPE_BOX;
  box->id = dCreateBox(0, x, y, z);
  dGeomSetData(box->id, box);
  return box;
}

void lovrBoxShapeGetDimensions(BoxShape* box, f32* x, f32* y, f32* z) {
  f32 dimensions[3];
  dGeomBoxGetLengths(box->id, dimensions);
  *x = dimensions[0];
  *y = dimensions[1];
  *z = dimensions[2];
}

void lovrBoxShapeSetDimensions(BoxShape* box, f32 x, f32 y, f32 z) {
  dGeomBoxSetLengths(box->id, x, y, z);
}

CapsuleShape* lovrCapsuleShapeInit(CapsuleShape* capsule, f32 radius, f32 length) {
  capsule->type = SHAPE_CAPSULE;
  capsule->id = dCreateCapsule(0, radius, length);
  dGeomSetData(capsule->id, capsule);
  return capsule;
}

f32 lovrCapsuleShapeGetRadius(CapsuleShape* capsule) {
  f32 radius, length;
  dGeomCapsuleGetParams(capsule->id, &radius, &length);
  return radius;
}

void lovrCapsuleShapeSetRadius(CapsuleShape* capsule, f32 radius) {
  dGeomCapsuleSetParams(capsule->id, radius, lovrCapsuleShapeGetLength(capsule));
}

f32 lovrCapsuleShapeGetLength(CapsuleShape* capsule) {
  f32 radius, length;
  dGeomCapsuleGetParams(capsule->id, &radius, &length);
  return length;
}

void lovrCapsuleShapeSetLength(CapsuleShape* capsule, f32 length) {
  dGeomCapsuleSetParams(capsule->id, lovrCapsuleShapeGetRadius(capsule), length);
}

CylinderShape* lovrCylinderShapeInit(CylinderShape* cylinder, f32 radius, f32 length) {
  cylinder->type = SHAPE_CYLINDER;
  cylinder->id = dCreateCylinder(0, radius, length);
  dGeomSetData(cylinder->id, cylinder);
  return cylinder;
}

f32 lovrCylinderShapeGetRadius(CylinderShape* cylinder) {
  f32 radius, length;
  dGeomCylinderGetParams(cylinder->id, &radius, &length);
  return radius;
}

void lovrCylinderShapeSetRadius(CylinderShape* cylinder, f32 radius) {
  dGeomCylinderSetParams(cylinder->id, radius, lovrCylinderShapeGetLength(cylinder));
}

f32 lovrCylinderShapeGetLength(CylinderShape* cylinder) {
  f32 radius, length;
  dGeomCylinderGetParams(cylinder->id, &radius, &length);
  return length;
}

void lovrCylinderShapeSetLength(CylinderShape* cylinder, f32 length) {
  dGeomCylinderSetParams(cylinder->id, lovrCylinderShapeGetRadius(cylinder), length);
}

void lovrJointDestroy(void* ref) {
  Joint* joint = ref;
  lovrJointDestroyData(joint);
}

void lovrJointDestroyData(Joint* joint) {
  if (joint->id) {
    dJointDestroy(joint->id);
    joint->id = NULL;
  }
}

JointType lovrJointGetType(Joint* joint) {
  return joint->type;
}

void lovrJointGetColliders(Joint* joint, Collider** a, Collider** b) {
  dBodyID bodyA = dJointGetBody(joint->id, 0);
  dBodyID bodyB = dJointGetBody(joint->id, 1);

  if (bodyA) {
    *a = dBodyGetData(bodyA);
  }

  if (bodyB) {
    *b = dBodyGetData(bodyB);
  }
}

void* lovrJointGetUserData(Joint* joint) {
  return joint->userdata;
}

void lovrJointSetUserData(Joint* joint, void* data) {
  joint->userdata = data;
}

BallJoint* lovrBallJointInit(BallJoint* joint, Collider* a, Collider* b, f32 x, f32 y, f32 z) {
  lovrAssert(a->world == b->world, "Joint bodies must exist in same World");
  joint->type = JOINT_BALL;
  joint->id = dJointCreateBall(a->world->id, 0);
  dJointSetData(joint->id, joint);
  dJointAttach(joint->id, a->body, b->body);
  lovrBallJointSetAnchor(joint, x, y, z);
  lovrRetain(joint);
  return joint;
}

void lovrBallJointGetAnchors(BallJoint* joint, f32* x1, f32* y1, f32* z1, f32* x2, f32* y2, f32* z2) {
  f32 anchor[3];
  dJointGetBallAnchor(joint->id, anchor);
  *x1 = anchor[0];
  *y1 = anchor[1];
  *z1 = anchor[2];
  dJointGetBallAnchor2(joint->id, anchor);
  *x2 = anchor[0];
  *y2 = anchor[1];
  *z2 = anchor[2];
}

void lovrBallJointSetAnchor(BallJoint* joint, f32 x, f32 y, f32 z) {
  dJointSetBallAnchor(joint->id, x, y, z);
}

DistanceJoint* lovrDistanceJointInit(DistanceJoint* joint, Collider* a, Collider* b, f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2) {
  lovrAssert(a->world == b->world, "Joint bodies must exist in same World");
  joint->type = JOINT_DISTANCE;
  joint->id = dJointCreateDBall(a->world->id, 0);
  dJointSetData(joint->id, joint);
  dJointAttach(joint->id, a->body, b->body);
  lovrDistanceJointSetAnchors(joint, x1, y1, z1, x2, y2, z2);
  lovrRetain(joint);
  return joint;
}

void lovrDistanceJointGetAnchors(DistanceJoint* joint, f32* x1, f32* y1, f32* z1, f32* x2, f32* y2, f32* z2) {
  f32 anchor[3];
  dJointGetDBallAnchor1(joint->id, anchor);
  *x1 = anchor[0];
  *y1 = anchor[1];
  *z1 = anchor[2];
  dJointGetDBallAnchor2(joint->id, anchor);
  *x2 = anchor[0];
  *y2 = anchor[1];
  *z2 = anchor[2];
}

void lovrDistanceJointSetAnchors(DistanceJoint* joint, f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2) {
  dJointSetDBallAnchor1(joint->id, x1, y1, z1);
  dJointSetDBallAnchor2(joint->id, x2, y2, z2);
}

f32 lovrDistanceJointGetDistance(DistanceJoint* joint) {
  return dJointGetDBallDistance(joint->id);
}

void lovrDistanceJointSetDistance(DistanceJoint* joint, f32 distance) {
  dJointSetDBallDistance(joint->id, distance);
}

HingeJoint* lovrHingeJointInit(HingeJoint* joint, Collider* a, Collider* b, f32 x, f32 y, f32 z, f32 ax, f32 ay, f32 az) {
  lovrAssert(a->world == b->world, "Joint bodies must exist in same World");
  joint->type = JOINT_HINGE;
  joint->id = dJointCreateHinge(a->world->id, 0);
  dJointSetData(joint->id, joint);
  dJointAttach(joint->id, a->body, b->body);
  lovrHingeJointSetAnchor(joint, x, y, z);
  lovrHingeJointSetAxis(joint, ax, ay, az);
  lovrRetain(joint);
  return joint;
}

void lovrHingeJointGetAnchors(HingeJoint* joint, f32* x1, f32* y1, f32* z1, f32* x2, f32* y2, f32* z2) {
  f32 anchor[3];
  dJointGetHingeAnchor(joint->id, anchor);
  *x1 = anchor[0];
  *y1 = anchor[1];
  *z1 = anchor[2];
  dJointGetHingeAnchor2(joint->id, anchor);
  *x2 = anchor[0];
  *y2 = anchor[1];
  *z2 = anchor[2];
}

void lovrHingeJointSetAnchor(HingeJoint* joint, f32 x, f32 y, f32 z) {
  dJointSetHingeAnchor(joint->id, x, y, z);
}

void lovrHingeJointGetAxis(HingeJoint* joint, f32* x, f32* y, f32* z) {
  f32 axis[3];
  dJointGetHingeAxis(joint->id, axis);
  *x = axis[0];
  *y = axis[1];
  *z = axis[2];
}

void lovrHingeJointSetAxis(HingeJoint* joint, f32 x, f32 y, f32 z) {
  dJointSetHingeAxis(joint->id, x, y, z);
}

f32 lovrHingeJointGetAngle(HingeJoint* joint) {
  return dJointGetHingeAngle(joint->id);
}

f32 lovrHingeJointGetLowerLimit(HingeJoint* joint) {
  return dJointGetHingeParam(joint->id, dParamLoStop);
}

void lovrHingeJointSetLowerLimit(HingeJoint* joint, f32 limit) {
  dJointSetHingeParam(joint->id, dParamLoStop, limit);
}

f32 lovrHingeJointGetUpperLimit(HingeJoint* joint) {
  return dJointGetHingeParam(joint->id, dParamHiStop);
}

void lovrHingeJointSetUpperLimit(HingeJoint* joint, f32 limit) {
  dJointSetHingeParam(joint->id, dParamHiStop, limit);
}

SliderJoint* lovrSliderJointInit(SliderJoint* joint, Collider* a, Collider* b, f32 ax, f32 ay, f32 az) {
  lovrAssert(a->world == b->world, "Joint bodies must exist in the same world");
  joint->type = JOINT_SLIDER;
  joint->id = dJointCreateSlider(a->world->id, 0);
  dJointSetData(joint->id, joint);
  dJointAttach(joint->id, a->body, b->body);
  lovrSliderJointSetAxis(joint, ax, ay, az);
  lovrRetain(joint);
  return joint;
}

void lovrSliderJointGetAxis(SliderJoint* joint, f32* x, f32* y, f32* z) {
  f32 axis[3];
  dJointGetSliderAxis(joint->id, axis);
  *x = axis[0];
  *y = axis[1];
  *z = axis[2];
}

void lovrSliderJointSetAxis(SliderJoint* joint, f32 x, f32 y, f32 z) {
  dJointSetSliderAxis(joint->id, x, y, z);
}

f32 lovrSliderJointGetPosition(SliderJoint* joint) {
  return dJointGetSliderPosition(joint->id);
}

f32 lovrSliderJointGetLowerLimit(SliderJoint* joint) {
  return dJointGetSliderParam(joint->id, dParamLoStop);
}

void lovrSliderJointSetLowerLimit(SliderJoint* joint, f32 limit) {
  dJointSetSliderParam(joint->id, dParamLoStop, limit);
}

f32 lovrSliderJointGetUpperLimit(SliderJoint* joint) {
  return dJointGetSliderParam(joint->id, dParamHiStop);
}

void lovrSliderJointSetUpperLimit(SliderJoint* joint, f32 limit) {
  dJointSetSliderParam(joint->id, dParamHiStop, limit);
}
