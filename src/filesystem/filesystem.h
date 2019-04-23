#include "util.h"
#include "lib/vec/vec.h"

#pragma once

#define LOVR_PATH_MAX 1024

extern const char lovrDirSep;

typedef int (*getDirectoryItemsCallback)(void* userdata, const char* dir, const char* file);

bool lovrFilesystemInit(const char* argExe, const char* argGame, const char* argRoot);
void lovrFilesystemDestroy(void);
bool lovrFilesystemCreateDirectory(const char* path);
bool lovrFilesystemGetAppdataDirectory(char* dest, usize size);
void lovrFilesystemGetDirectoryItems(const char* path, getDirectoryItemsCallback callback, void* userdata);
bool lovrFilesystemGetExecutablePath(char* path, usize size);
const char* lovrFilesystemGetIdentity(void);
i64 lovrFilesystemGetLastModified(const char* path);
const char* lovrFilesystemGetRealDirectory(const char* path);
vec_str_t* lovrFilesystemGetRequirePath(void);
vec_str_t* lovrFilesystemGetCRequirePath(void);
const char* lovrFilesystemGetSaveDirectory(void);
usize lovrFilesystemGetSize(const char* path);
const char* lovrFilesystemGetSource(void);
const char* lovrFilesystemGetUserDirectory(void);
bool lovrFilesystemGetWorkingDirectory(char* dest, usize size);
bool lovrFilesystemIsDirectory(const char* path);
bool lovrFilesystemIsFile(const char* path);
bool lovrFilesystemIsFused(void);
bool lovrFilesystemMount(const char* path, const char* mountpoint, bool append, const char *root);
void* lovrFilesystemRead(const char* path, usize bytes, usize* bytesRead);
bool lovrFilesystemRemove(const char* path);
bool lovrFilesystemSetIdentity(const char* identity);
void lovrFilesystemSetRequirePath(const char* requirePath);
void lovrFilesystemSetCRequirePath(const char* requirePath);
bool lovrFilesystemUnmount(const char* path);
usize lovrFilesystemWrite(const char* path, const char* content, usize size, bool append);
