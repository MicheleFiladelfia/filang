#include <string.h>
#include "hashmap.h"
#include "memory.h"
#include "value.h"
#include "strings.h"

uint32_t hashString(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }
    return hash;
}

void initHashMap(HashMap *map) {
    map->count = 0;
    map->capacity = 0;
    map->entries = NULL;
}

void freeHashMap(HashMap *map) {
    FREE_ARRAY(map->entries, Entry, map->capacity);
    initHashMap(map);
}

void growCapacity(HashMap *map) {
    uint32_t oldCapacity = map->capacity;
    Entry *oldEntries = map->entries;
    map->capacity = GROW_ARRAY_CAPACITY(oldCapacity);
    map->entries = ALLOCATE(Entry, map->capacity);
    for (int i = 0; i < map->capacity; i++) {
        map->entries[i].key = NULL;
        map->entries[i].value = NIL;
    }

    map->count = 0;
    for (int i = 0; i < oldCapacity; i++) {
        Entry *entry = &oldEntries[i];
        if (entry->key == NULL) continue;
        addEntry(map, entry->key, entry->value);
    }
    FREE_ARRAY(oldEntries, Entry, oldCapacity);
}

static void removeByIndex(HashMap *map, uint32_t index) {
    while (true) {
        map->entries[index].key = NULL;
        uint32_t next = (index + 1) & (map->capacity - 1);
        if (IS_EMPTY(map->entries[next])) return;
        uint32_t desired = map->entries[next].key->hash & (map->capacity - 1);
        if (next == desired) return;
        map->entries[index].key = map->entries[next].key;
        map->entries[index].value = map->entries[next].value;
        index = next;
    }
}

void eraseEntry(HashMap *map, ObjString *key) {
    if (map->count == 0) return;
    uint32_t index = key->hash & (map->capacity - 1);

    while (true) {
        if (IS_EMPTY(map->entries[index])) {
            return;
        } else if (map->entries[index].key == key) {
            removeByIndex(map, index);
            map->count--;
            return;
        }
        index = (index + 1) & (map->capacity - 1);
    }
}

bool contains(HashMap *map, ObjString *key) {
    if (map->count == 0) return false;

    uint32_t hash = key->hash;
    uint64_t index = hash & (map->capacity - 1);
    uint64_t dist = 0;
    for (;;) {
        if (IS_EMPTY(map->entries[index])) return false;
        if (map->entries[index].key == key) return true;
        uint64_t desired = map->entries[index].key->hash & (map->capacity - 1);
        uint64_t cur_dist = (index + map->capacity - desired) & (map->capacity - 1);
        if (cur_dist < dist) return false;
        dist++;
        index = (index + 1) & (map->capacity - 1);
    }
}

//assuming the key is not already in the map
bool addEntry(HashMap *map, ObjString *key, Value value) {
    if (map->count + 1 > map->capacity * HASHMAP_MAX_LOAD) {
        growCapacity(map);
    }

    uint32_t index = key->hash & (map->capacity - 1);
    uint32_t dist = 0;
    map->count++;

    for (;;) {
        if (IS_EMPTY(map->entries[index])) {
            map->entries[index].key = key;
            map->entries[index].value = value;
            return false;
        }else if (map->entries[index].key == key) {
            map->entries[index].value = value;
            return true;
        }

        uint32_t desired = map->entries[index].key->hash & (map->capacity - 1);
        uint32_t cur_dist = (index + map->capacity - desired) & (map->capacity - 1);
        if (cur_dist < dist) {
            //swapping current key and value with the collided key and value
            ObjString *temp_key = map->entries[index].key;
            Value temp_value = map->entries[index].value;
            map->entries[index].key = key;
            map->entries[index].value = value;
            key = temp_key;
            value = temp_value;

            dist = cur_dist;
        }
        dist++;
        index = (index + 1) & (map->capacity - 1);

    }
}


Value getEntry(HashMap *map, ObjString *key) {
    uint32_t hash = key->hash;
    uint64_t index = hash & (map->capacity - 1);

    for (;;) {
        if (map->count == 0 || IS_EMPTY(map->entries[index])) {
            return NIL;
        }

        if (map->entries[index].key == key) {
            return map->entries[index].value;
        }

        index = (index + 1) & (map->capacity - 1);
    }
}

ObjString *getStringEntry(HashMap *map, const char *key, int length, uint32_t hash) {
    uint64_t index = hash & (map->capacity - 1);

    for (;;) {
        if (map->count == 0 || IS_EMPTY(map->entries[index])) {
            return NULL;
        }

        if (length == map->entries[index].key->length &&
            (hash == map->entries[index].key->hash) &&
            (memcmp(key, map->entries[index].key->chars, length) == 0)) {
            return map->entries[index].key;
        }

        index = (index + 1) & (map->capacity - 1);
    }
}

