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

uint32_t hashDouble(double key) {
    const uint32_t fnvOffset = 2166136261u;
    const uint32_t fnvPrime = 16777619u;
    uint32_t hash = fnvOffset;
    uint8_t *bytes = (uint8_t *) &key;
    for (size_t i = 0; i < sizeof(double); i++) {
        hash ^= bytes[i];
        hash *= fnvPrime;
    }
    return hash;
}

uint32_t hashInt(int64_t key) {
    const uint32_t fnvOffset = 2166136261u;
    const uint32_t fnvPrime = 16777619u;
    uint32_t hash = fnvOffset;
    uint8_t *bytes = (uint8_t *) &key;
    for (size_t i = 0; i < sizeof(key); i++) {
        hash ^= bytes[i];
        hash *= fnvPrime;
    }
    return hash;
}

static bool compare(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL:
        case VAL_INTEGER:
            return a.as.integer == b.as.integer;
        case VAL_FLOAT:
            return a.as.floatingPoint == b.as.floatingPoint;
        case VAL_OBJECT:
            if (IS_STRING(a) && IS_STRING(b)) {
                return AS_STRING(a) == AS_STRING(b);
            }
            return false;
        default:
            return false;
    }
}

uint32_t getHash(Value val) {
    switch (val.type) {
        case VAL_BOOL:
        case VAL_INTEGER:
            return hashInt(val.as.integer);
        case VAL_FLOAT:
            return hashDouble(val.as.floatingPoint);
        case VAL_OBJECT:
            if (IS_STRING(val)) {
                return AS_STRING(val)->hash;
            }
            return (uint32_t) (intptr_t) val.as.object;
        default:
            return 0;
    }
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
        map->entries[i].key.type = VAL_NIL;
        map->entries[i].value = NIL;
    }

    map->count = 0;
    for (int i = 0; i < (int)oldCapacity; i++) {
        Entry *entry = &oldEntries[i];
        if (entry->key.type == VAL_NIL) continue;
        addEntry(map, entry->key, entry->value);
    }
    FREE_ARRAY(oldEntries, Entry, oldCapacity);
}

static void removeByIndex(HashMap *map, uint32_t index) {
    while (true) {
        map->entries[index].key.type = VAL_NIL;
        uint32_t next = (index + 1) & (map->capacity - 1);
        if (IS_EMPTY(map->entries[next])) return;
        uint32_t desired = getHash(map->entries[next].key) & (map->capacity - 1);
        if (next == desired) return;
        map->entries[index].key = map->entries[next].key;
        map->entries[index].value = map->entries[next].value;
        index = next;
    }
}

void eraseEntry(HashMap *map, Value key) {
    if (map->count == 0) return;
    uint32_t index = getHash(key) & (map->capacity - 1);

    while (true) {
        if (IS_EMPTY(map->entries[index])) {
            return;
        } else if (compare(map->entries[index].key, key)) {
            removeByIndex(map, index);
            map->count--;
            return;
        }
        index = (index + 1) & (map->capacity - 1);
    }
}

bool contains(HashMap *map, Value key) {
    if (map->count == 0) return false;

    uint32_t hash = getHash(key);
    uint64_t index = hash & (map->capacity - 1);
    uint64_t dist = 0;
    for (;;) {
        if (IS_EMPTY(map->entries[index])) return false;
        if (compare(map->entries[index].key, key)) return true;
        uint64_t desired = getHash(map->entries[index].key) & (map->capacity - 1);
        uint64_t cur_dist = (index + map->capacity - desired) & (map->capacity - 1);
        if (cur_dist < dist) return false;
        dist++;
        index = (index + 1) & (map->capacity - 1);
    }
}

bool addEntry(HashMap *map, Value key, Value value) {
    if (map->count + 1 > map->capacity * HASHMAP_MAX_LOAD) {
        growCapacity(map);
    }

    uint32_t index = getHash(key) & (map->capacity - 1);
    uint32_t dist = 0;
    map->count++;

    for (;;) {
        if (IS_EMPTY(map->entries[index])) {
            map->entries[index].key = key;
            map->entries[index].value = value;
            return false;
        } else if (compare(map->entries[index].key, key)) {
            //replacing the value
            map->entries[index].value = value;
            return true;
        }

        uint32_t desired = getHash(map->entries[index].key) & (map->capacity - 1);
        uint32_t cur_dist = (index + map->capacity - desired) & (map->capacity - 1);
        if (cur_dist < dist) {
            //swapping current key and value with the collided key and value
            Value temp_key = map->entries[index].key;
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


Entry *getEntry(HashMap *map, Value key) {
    uint32_t hash = getHash(key);
    uint64_t index = hash & (map->capacity - 1);

    for (;;) {
        if (map->count == 0 || IS_EMPTY(map->entries[index])) {
            return NULL;
        }

        if (compare(map->entries[index].key, key)) {
            return &map->entries[index];
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

        if (length == AS_STRING(map->entries[index].key)->length &&
            (hash == AS_STRING(map->entries[index].key)->hash) &&
            (memcmp(key, AS_STRING(map->entries[index].key)->chars, length) == 0)) {
            return AS_STRING(map->entries[index].key);
        }

        index = (index + 1) & (map->capacity - 1);
    }
}

