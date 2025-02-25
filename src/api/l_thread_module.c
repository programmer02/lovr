#include "api.h"
#include "event/event.h"
#include "filesystem/filesystem.h"
#include "thread/thread.h"
#include "thread/channel.h"
#include "core/ref.h"
#include <stdlib.h>
#include <string.h>

static int threadRunner(void* data) {
  Thread* thread = (Thread*) data;

  lovrRetain(thread);
  mtx_lock(&thread->lock);
  thread->running = true;
  mtx_unlock(&thread->lock);

  lua_State* L = luaL_newstate();
  luaL_openlibs(L);
  lovrSetErrorCallback((errorFn*) luax_vthrow, L);

  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  luaL_register(L, NULL, lovrModules);
  lua_pop(L, 2);

  if (luaL_loadbuffer(L, thread->body->data, thread->body->size, "thread") || lua_pcall(L, 0, 0, 0)) {
    size_t length;
    const char* error = lua_tolstring(L, -1, &length);
    mtx_lock(&thread->lock);
    thread->error = malloc(length + 1);
    if (thread->error) {
      memcpy(thread->error, error, length + 1);
      lovrEventPush((Event) {
        .type = EVENT_THREAD_ERROR,
        .data.thread = { thread, thread->error }
      });
    }
    thread->running = false;
    mtx_unlock(&thread->lock);
    lovrRelease(Thread, thread);
    lua_close(L);
    return 1;
  }

  mtx_lock(&thread->lock);
  thread->running = false;
  mtx_unlock(&thread->lock);
  lovrRelease(Thread, thread);
  lua_close(L);
  return 0;
}

static int l_lovrThreadNewThread(lua_State* L) {
  Blob* blob = luax_totype(L, 1, Blob);
  if (!blob) {
    size_t length;
    const char* str = lua_tolstring(L, 1, &length);
    if (memchr(str, '\n', MIN(1024, length))) {
      void* data = malloc(length + 1);
      lovrAssert(data, "Out of memory");
      memcpy(data, str, length + 1);
      blob = lovrBlobCreate(data, length, "thread code");
    } else {
      void* code = lovrFilesystemRead(str, -1, &length);
      lovrAssert(code, "Could not read thread code from file '%s'", str);
      blob = lovrBlobCreate(code, length, str);
    }
  } else {
    lovrRetain(blob);
  }
  Thread* thread = lovrThreadCreate(threadRunner, blob);
  luax_pushtype(L, Thread, thread);
  lovrRelease(Thread, thread);
  lovrRelease(Blob, blob);
  return 1;
}

static int l_lovrThreadGetChannel(lua_State* L) {
  const char* name = luaL_checkstring(L, 1);
  Channel* channel = lovrThreadGetChannel(name);
  luax_pushtype(L, Channel, channel);
  return 1;
}

static const luaL_Reg lovrThreadModule[] = {
  { "newThread", l_lovrThreadNewThread },
  { "getChannel", l_lovrThreadGetChannel },
  { NULL, NULL }
};

int luaopen_lovr_thread(lua_State* L) {
  lua_newtable(L);
  luaL_register(L, NULL, lovrThreadModule);
  luax_registertype(L, Thread);
  luax_registertype(L, Channel);
  if (lovrThreadModuleInit()) {
    luax_atexit(L, lovrThreadModuleDestroy);
  }
  return 1;
}
