#include "util.h"
#include "event/event.h"
#include "types.h"
#include "lib/tinycthread/tinycthread.h"
#include "lib/vec/vec.h"

#pragma once

typedef struct Channel {
  Ref ref;
  mtx_t lock;
  cnd_t cond;
  vec_t(Variant) messages;
  u64 sent;
  u64 received;
} Channel;

Channel* lovrChannelInit(Channel* channel);
#define lovrChannelCreate() lovrChannelInit(lovrAlloc(Channel))
void lovrChannelDestroy(void* ref);
bool lovrChannelPush(Channel* channel, Variant variant, f64 timeout, u64* id);
bool lovrChannelPop(Channel* channel, Variant* variant, f64 timeout);
bool lovrChannelPeek(Channel* channel, Variant* variant);
void lovrChannelClear(Channel* channel);
u64 lovrChannelGetCount(Channel* channel);
bool lovrChannelHasRead(Channel* channel, u64 id);
