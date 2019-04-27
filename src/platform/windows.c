#include "platform.h"
#include <Windows.h>

#include "platform/glfw.h"
#include "platform/log.c"

const char* lovrPlatformGetName() {
  return "Windows";
}

void lovrPlatformSleep(double seconds) {
  Sleep((unsigned int) (seconds * 1000));
}

int lovrPlatformGetExecutablePath(char* dest, uint32_t size) {
  return !GetModuleFileName(NULL, dest, size);
}

sds lovrPlatformGetApplicationId() {
	return NULL;
}
