#include "err.h"
#include <stdlib.h>
#include <stdio.h>

ERR_THREAD err_fn err_handler;
ERR_THREAD void* err_context;

void err_setHandler(err_fn handler, void* context) {
  err_handler = handler;
  err_context = context;
}

void lovrThrow(const char* format, ...) {
  if (err_handler) {
    va_list args;
    va_start(args, format);
    err_handler(err_context, format, args);
    va_end(args);
    abort();
  } else {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    fflush(NULL);
    abort();
  }
}
