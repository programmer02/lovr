#include "platform.h"
#include <emscripten.h>

#include "platform_glfw.c.h"

const char* lovrPlatformGetName() {
  return "Web";
}

void lovrPlatformSleep(double seconds) {
  emscripten_sleep((unsigned int) (seconds * 1000));
}

int lovrPlatformGetExecutablePath(char* dest, uint32_t size) {
  return 1;
}

void lovrPlatformOpenConsole() {
  //
}
