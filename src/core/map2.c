#include "map.h"
#include <stdlib.h>
#include <string.h>
#include "util.h"

typedef union {
  uint64_t u;
  void* p;
} map_val;

static void map_rehash(map* m) {
  map old = *m;
  m->size <<= 1;
  m->hashes = malloc(2 * m->size * sizeof(uint64_t));
  m->values = m->hashes + m->size;
  lovrAssert(m->hashes, "Out of memory");
  memset(m->hashes, 0xff, 2 * m->size * sizeof(uint64_t));

  if (old.hashes) {
    for (uint32_t i = 0; i < old.size; i++) {
      if (old.hashes[i] != MAP_NIL) {
        uint64_t index = old.hashes[i] & (m->size - 1);
        m->hashes[index] = old.hashes[i];
        m->values[index] = old.values[i];
      }
    }

    free(old.hashes);
  }
}

static inline uint64_t map_find(map* map, uint64_t hash) {
  uint64_t mask = map->size - 1;
  uint64_t h = hash & mask;

  while (map->hashes[h] != hash && map->hashes[h] != MAP_NIL) {
    h = (h + 1) & mask;
  }

  return h;
}

void map_init(map* map, uint32_t n) {
  map->size = n;
  map->used = 0;
  map->hashes = NULL;
  map_rehash(map);
}

void map_free(map* map) {
  free(map->hashes);
}

uint64_t map_get(map* map, uint64_t hash) {
  uint64_t h = map_find(map, hash);
  return map->hashes[h] == MAP_NIL ? MAP_NIL : map->values[h];
}

void* map_getp(map* map, uint64_t hash) {
  map_val v = { map_get(map, hash) };
  return v.p;
}

void map_set(map* map, uint64_t hash, uint64_t value) {
  if (map->used >= (map->size >> 1) + (map->size >> 2)) {
    map_rehash(map);
  }

  uint64_t h = map_find(map, hash);
  lovrAssert(map->hashes[h] == MAP_NIL, "Collision!");

  map->used++;
  map->hashes[h] = hash;
  map->values[h] = value;
}

void map_setp(map* map, uint64_t hash, void* value) {
  map_val v = { .p = value };
  map_set(map, hash, v.u);
}
