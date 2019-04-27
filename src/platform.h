#include "util.h"
#include <lib/sds/sds.h>

#pragma once

typedef struct {
  u32 width;
  u32 height;
  bool fullscreen;
  bool srgb;
  int vsync;
  int msaa;
  const char* title;
  struct {
    void* data;
    u32 width;
    u32 height;
  } icon;
} WindowFlags;

typedef enum {
  MOUSE_LEFT,
  MOUSE_RIGHT
} MouseButton;

typedef enum {
  MOUSE_MODE_NORMAL,
  MOUSE_MODE_GRABBED
} MouseMode;

typedef enum {
  KEY_W,
  KEY_A,
  KEY_S,
  KEY_D,
  KEY_Q,
  KEY_E,
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT
} KeyCode;

typedef enum {
  BUTTON_PRESSED,
  BUTTON_RELEASED
} ButtonAction;

typedef void (*windowCloseCallback)(void);
typedef void (*windowResizeCallback)(u32 width, u32 height);
typedef void (*mouseButtonCallback)(MouseButton button, ButtonAction action);

typedef void (*gpuProc)(void);
typedef gpuProc (*getProcAddressProc)(const char*);
extern getProcAddressProc lovrGetProcAddress;

void lovrLog(const char* format, ...);
void lovrLogv(const char* format, va_list args);
void lovrWarn(const char* format, ...);
void lovrWarnv(const char* format, va_list args);

bool lovrPlatformInit(void);
void lovrPlatformDestroy(void);
void lovrPlatformPollEvents(void);
double lovrPlatformGetTime(void);
void lovrPlatformSetTime(double t);
bool lovrPlatformCreateWindow(WindowFlags* flags);
bool lovrPlatformHasWindow(void);
void lovrPlatformGetWindowSize(u32* width, u32* height);
void lovrPlatformGetFramebufferSize(u32* width, u32* height);
void lovrPlatformSwapBuffers(void);
void lovrPlatformOnWindowClose(windowCloseCallback callback);
void lovrPlatformOnWindowResize(windowResizeCallback callback);
void lovrPlatformOnMouseButton(mouseButtonCallback callback);
void lovrPlatformGetMousePosition(double* x, double* y);
void lovrPlatformSetMouseMode(MouseMode mode);
bool lovrPlatformIsMouseDown(MouseButton button);
bool lovrPlatformIsKeyDown(KeyCode key);
void lovrPlatformSleep(double seconds);
int lovrPlatformGetExecutablePath(char* dest, uint32_t size);
sds lovrPlatformGetApplicationId(void);
