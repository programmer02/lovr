#include "types.h"
#include "util.h"

#pragma once

typedef enum {
  OPEN_READ,
  OPEN_WRITE,
  OPEN_APPEND
} FileMode;

typedef struct {
  Ref ref;
  const char* path;
  void* handle;
  FileMode mode;
} File;

File* lovrFileInit(File* file, const char* filename);
#define lovrFileCreate(...) lovrFileInit(lovrAlloc(File), __VA_ARGS__)
void lovrFileDestroy(void* ref);
bool lovrFileOpen(File* file, FileMode mode);
void lovrFileClose(File* file);
usize lovrFileRead(File* file, void* data, usize bytes);
usize lovrFileWrite(File* file, const void* data, usize bytes);
usize lovrFileGetSize(File* file);
bool lovrFileSeek(File* file, usize position);
usize lovrFileTell(File* file);
