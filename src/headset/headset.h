#include "util.h"
#include "types.h"

#pragma once

struct ModelData;
struct Texture;

typedef enum {
  ORIGIN_HEAD,
  ORIGIN_FLOOR
} HeadsetOrigin;

typedef enum {
  DRIVER_DESKTOP,
  DRIVER_OCULUS,
  DRIVER_OCULUS_MOBILE,
  DRIVER_OPENVR,
  DRIVER_WEBVR
} HeadsetDriver;

typedef struct HeadsetInterface {
  struct HeadsetInterface* next;
  HeadsetDriver driverType;
  bool (*init)(f32 offset, u32 msaa);
  void (*destroy)(void);
  bool (*getName)(char* name, usize length);
  HeadsetOrigin (*getOriginType)(void);
  void (*getDisplayDimensions)(u32* width, u32* height);
  void (*getClipDistance)(f32* clipNear, f32* clipFar);
  void (*setClipDistance)(f32 clipNear, f32 clipFar);
  void (*getBoundsDimensions)(f32* width, f32* depth);
  const f32* (*getBoundsGeometry)(u8* count);
  bool (*getPose)(const char* path, f32* x, f32* y, f32* z, f32* angle, f32* ax, f32* ay, f32* az);
  bool (*getVelocity)(const char* path, f32* vx, f32* vy, f32* vz, f32* vax, f32* vay, f32* vaz);
  bool (*isDown)(const char* path, bool* down);
  bool (*isTouched)(const char* path, bool* touched);
  u8 (*getAxis)(const char* path, f32* x, f32* y, f32* z);
  bool (*vibrate)(const char* path, f32 strength, f32 duration, f32 frequency);
  struct ModelData* (*newModelData)(const char* path);
  void (*renderTo)(void (*callback)(void*), void* userdata);
  struct Texture* (*getMirrorTexture)(void);
  void (*update)(f32 dt);
} HeadsetInterface;

// Available drivers
extern HeadsetInterface lovrHeadsetOculusDriver;
extern HeadsetInterface lovrHeadsetOpenVRDriver;
extern HeadsetInterface lovrHeadsetWebVRDriver;
extern HeadsetInterface lovrHeadsetDesktopDriver;
extern HeadsetInterface lovrHeadsetOculusMobileDriver;

// Active drivers
extern HeadsetInterface* lovrHeadsetDriver;
extern HeadsetInterface* lovrHeadsetTrackingDrivers;

#define FOREACH_TRACKING_DRIVER(i)\
  for (HeadsetInterface* i = lovrHeadsetTrackingDrivers; i != NULL; i = i->next)

bool lovrHeadsetInit(HeadsetDriver* drivers, u8 count, f32 offset, u32 msaa);
void lovrHeadsetDestroy(void);
