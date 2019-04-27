#ifdef __ANDROID__
#include <android/log.h>
void lovrLogv(const char* format, va_list args) { __android_log_vprint(ANDROID_LOG_DEBUG, "LOVR", args); }
void lovrWarnv(const char* format, va_list args) { __android_log_vprint(ANDROID_LOG_WARN, "LOVR", args); }
#else
#include <stdio.h>
void lovrLogv(const char* format, va_list args) { vprintf(format, args); }
void lovrWarnv(const char* format, va_list args) { vfprintf(stderr, format, args); }
#endif

void lovrLog(const char* format, ...) {
  va_list args;
  va_start(args, format);
  lovrLogv(format, args);
  va_end(args);
}

void lovrWarn(const char* format, ...) {
  va_list args;
  va_start(args, format);
  lovrWarnv(format, args);
  va_end(args);
}
