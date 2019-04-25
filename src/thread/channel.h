#include "util.h"

#pragma once

struct Variant;

typedef struct Channel Channel;
extern const usize sizeof_Channel;
Channel* lovrChannelInit(Channel* channel);
void lovrChannelDestroy(void* ref);
bool lovrChannelPush(Channel* channel, struct Variant* variant, f64 timeout, u64* id);
bool lovrChannelPop(Channel* channel, struct Variant* variant, f64 timeout);
bool lovrChannelPeek(Channel* channel, struct Variant* variant);
void lovrChannelClear(Channel* channel);
i32 lovrChannelGetCount(Channel* channel);
bool lovrChannelHasRead(Channel* channel, u64 id);
