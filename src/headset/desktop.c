#include "headset/headset.h"
#include "graphics/graphics.h"
#include "lib/maf.h"
#include "platform.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

static struct {
  f32 offset;

  f32 clipNear;
  f32 clipFar;

  f32 position[3];
  f32 velocity[3];
  f32 localVelocity[3];
  f32 angularVelocity[3];

  f32 yaw;
  f32 pitch;
  f32 transform[16];

  f64 prevCursorX;
  f64 prevCursorY;
} state;

static bool init(f32 offset, u32 msaa) {
  state.offset = offset;
  state.clipNear = .1f;
  state.clipFar = 100.f;
  mat4_identity(state.transform);
  return true;
}

static void destroy(void) {
  memset(&state, 0, sizeof(state));
}

static bool getName(char* name, usize length) {
  strncpy(name, "Simulator", length - 1);
  name[length - 1] = '\0';
  return true;
}

static HeadsetOrigin getOriginType(void) {
  return ORIGIN_HEAD;
}

static void getDisplayDimensions(u32* width, u32* height) {
  lovrPlatformGetFramebufferSize(width, height);
}

static void getClipDistance(f32* clipNear, f32* clipFar) {
  *clipNear = state.clipNear;
  *clipFar = state.clipFar;
}

static void setClipDistance(f32 clipNear, f32 clipFar) {
  state.clipNear = clipNear;
  state.clipFar = clipFar;
}

static void getBoundsDimensions(f32* width, f32* depth) {
  *width = *depth = 0.f;
}

static const f32* getBoundsGeometry(u32* count) {
  *count = 0;
  return NULL;
}

static bool getPose(const char* path, f32* x, f32* y, f32* z, f32* angle, f32* ax, f32* ay, f32* az) {
  bool head = !strcmp(path, "head");
  bool hand = !strcmp(path, "hand/left") || !strcmp(path, "hand/right");

  if (!head && !hand) {
    return false;
  }

  if (x) {
    *x = *y = 0.f;
    *z = hand ? -.75f : 0.f;
    mat4_transform(state.transform, x, y, z);
  }

  if (angle) {
    f32 q[4];
    quat_fromMat4(q, state.transform);
    quat_getAngleAxis(q, angle, ax, ay, az);
  }

  return true;
}

static bool getVelocity(const char* path, f32* vx, f32* vy, f32* vz, f32* vax, f32* vay, f32* vaz) {
  if (strcmp(path, "head")) {
    return false;
  }

  if (vx) {
    *vx = state.velocity[0];
    *vy = state.velocity[1];
    *vz = state.velocity[2];
  }

  if (vax) {
    *vax = state.angularVelocity[0];
    *vay = state.angularVelocity[1];
    *vaz = state.angularVelocity[2];
  }

  return true;
}

static bool isDown(const char* path, bool* down) {
  if (!strcmp(path, "hand/left") || !strcmp(path, "hand/right")) {
    *down = lovrPlatformIsMouseDown(MOUSE_RIGHT);
    return true;
  }

  return false;
}

static bool isTouched(const char* path, bool* touched) {
  return false;
}

static u32 getAxis(const char* path, f32* x, f32* y, f32* z) {
  return 0;
}

static bool vibrate(const char* path, f32 strength, f32 duration, f32 frequency) {
  return false;
}

static struct ModelData* newModelData(const char* path) {
  return NULL;
}

static void renderTo(void (*callback)(void*), void* userdata) {
  u32 width, height;
  getDisplayDimensions(&width, &height);
  Camera camera = { .canvas = NULL, .viewMatrix = { MAT4_IDENTITY }, .stereo = true };
  mat4_perspective(camera.projection[0], state.clipNear, state.clipFar, 67.f * PI / 180.f, (f32) width / 2.f / height);
  mat4_multiply(camera.viewMatrix[0], state.transform);
  mat4_invertPose(camera.viewMatrix[0]);
  mat4_set(camera.projection[1], camera.projection[0]);
  mat4_set(camera.viewMatrix[1], camera.viewMatrix[0]);
  lovrGraphicsSetCamera(&camera, true);
  callback(userdata);
  lovrGraphicsSetCamera(NULL, false);
}

static void update(f32 dt) {
  bool front = lovrPlatformIsKeyDown(KEY_W) || lovrPlatformIsKeyDown(KEY_UP);
  bool back = lovrPlatformIsKeyDown(KEY_S) || lovrPlatformIsKeyDown(KEY_DOWN);
  bool left = lovrPlatformIsKeyDown(KEY_A) || lovrPlatformIsKeyDown(KEY_LEFT);
  bool right = lovrPlatformIsKeyDown(KEY_D) || lovrPlatformIsKeyDown(KEY_RIGHT);
  bool up = lovrPlatformIsKeyDown(KEY_Q);
  bool down = lovrPlatformIsKeyDown(KEY_E);

  f32 movespeed = 3.f * dt;
  f32 turnspeed = 3.f * dt;
  f32 damping = MAX(1.f - 20.f * dt, 0.f);

  if (lovrPlatformIsMouseDown(MOUSE_LEFT)) {
    lovrPlatformSetMouseMode(MOUSE_MODE_GRABBED);

    u32 width, height;
    lovrPlatformGetWindowSize(&width, &height);

    f64 mx, my;
    lovrPlatformGetMousePosition(&mx, &my);

    if (state.prevCursorX == -1 && state.prevCursorY == -1) {
      state.prevCursorX = mx;
      state.prevCursorY = my;
    }

    f32 aspect = (f32) width / height;
    f32 dx = (f32) (mx - state.prevCursorX) / ((f32) width);
    f32 dy = (f32) (my - state.prevCursorY) / ((f32) height * aspect);
    state.angularVelocity[0] = dy / dt;
    state.angularVelocity[1] = dx / dt;
    state.prevCursorX = mx;
    state.prevCursorY = my;
  } else {
    lovrPlatformSetMouseMode(MOUSE_MODE_NORMAL);
    vec3_scale(state.angularVelocity, damping);
    state.prevCursorX = state.prevCursorY = -1;
  }

  // Update velocity
  state.localVelocity[0] = left ? -movespeed : (right ? movespeed : state.localVelocity[0]);
  state.localVelocity[1] = up ? movespeed : (down ? -movespeed : state.localVelocity[1]);
  state.localVelocity[2] = front ? -movespeed : (back ? movespeed : state.localVelocity[2]);
  vec3_init(state.velocity, state.localVelocity);
  mat4_transformDirection(state.transform, &state.velocity[0], &state.velocity[1], &state.velocity[2]);
  vec3_scale(state.localVelocity, damping);

  // Update position
  vec3_add(state.position, state.velocity);

  // Update orientation
  state.pitch = CLAMP(state.pitch - state.angularVelocity[0] * turnspeed, -PI / 2.f, PI / 2.f);
  state.yaw -= state.angularVelocity[1] * turnspeed;

  // Update transform
  mat4_identity(state.transform);
  mat4_translate(state.transform, 0.f, state.offset, 0.f);
  mat4_translate(state.transform, state.position[0], state.position[1], state.position[2]);
  mat4_rotate(state.transform, state.yaw, 0.f, 1.f, 0.f);
  mat4_rotate(state.transform, state.pitch, 1.f, 0.f, 0.f);
}

HeadsetInterface lovrHeadsetDesktopDriver = {
  .driverType = DRIVER_DESKTOP,
  .init = init,
  .destroy = destroy,
  .getName = getName,
  .getOriginType = getOriginType,
  .getDisplayDimensions = getDisplayDimensions,
  .getClipDistance = getClipDistance,
  .setClipDistance = setClipDistance,
  .getBoundsDimensions = getBoundsDimensions,
  .getBoundsGeometry = getBoundsGeometry,
  .getPose = getPose,
  .getVelocity = getVelocity,
  .isDown = isDown,
  .isTouched = isTouched,
  .getAxis = getAxis,
  .vibrate = vibrate,
  .newModelData = newModelData,
  .renderTo = renderTo,
  .update = update
};
