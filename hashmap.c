#include <string.h>
#include "hashmap.h"
#include "memory.h"
#include "value.h"
#include "strings.h"


uint32_t hash_string(const char *key, int length) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < length; i++) {
        hash ^= key[i];
        hash *= 16777619;
    }
    return hash;
}

uint32_t hash_double(double key) {
    const uint32_t fnv_offset = 2166136261u;
    const uint32_t fnv_prime = 16777619u;
    uint32_t hash = fnv_offset;
    uint8_t *bytes = (uint8_t *) &key;
    for (size_t i = 0; i < sizeof(double); i++) {
        hash ^= bytes[i];
        hash *= fnv_prime;
    }
    return hash;
}

uint32_t hash_int(int64_t key) {
    const uint32_t fnv_offset = 2166136261u;
    const uint32_t fnv_prime = 16777619u;
    uint32_t hash = fnv_offset;
    uint8_t *bytes = (uint8_t *) &key;
    for (size_t i = 0; i < sizeof(key); i++) {
        hash ^= bytes[i];
        hash *= fnv_prime;
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

uint32_t get_hash(Value val) {
    switch (val.type) {
        case VAL_BOOL:
        case VAL_INTEGER:
            return hash_int(val.as.integer);
        case VAL_FLOAT:
            return hash_double(val.as.floatingPoint);
        case VAL_OBJECT:
            if (IS_STRING(val)) {
                return AS_STRING(val)->hash;
            }
            return (uint32_t) (intptr_t) val.as.object;
        default:
            return 0;
    }
}

void init_hashmap(Hashmap *map) {
    map->count = 0;
    map->capacity = 0;
    map->entries = NULL;
}

void free_hashmap(Hashmap *map) {
    FREE_ARRAY(map->entries, Entry, map->capacity);
    init_hashmap(map);
}

void grow_capacity(Hashmap *map) {
    uint32_t old_capacity = map->capacity;
    Entry *old_entries = map->entries;
    map->capacity = GROW_ARRAY_CAPACITY(old_capacity);
    map->entries = ALLOCATE(Entry, map->capacity);
    for (int i = 0; i < map->capacity; i++) {
        map->entries[i].key.type = VAL_NIL;
        map->entries[i].value = NIL;
    }

    map->count = 0;
    for (int i = 0; i < (int) old_capacity; i++) {
        Entry *entry = &old_entries[i];
        if (entry->key.type == VAL_NIL) continue;
        add_entry(map, entry->key, entry->value);
    }
    FREE_ARRAY(old_entries, Entry, old_capacity);
}

static void remove_by_index(Hashmap *map, uint32_t index) {
    while (true) {
        map->entries[index].key.type = VAL_NIL;
        uint32_t next = (index + 1) & (map->capacity - 1);
        if (IS_EMPTY(map->entries[next])) return;
        uint32_t desired = get_hash(map->entries[next].key) & (map->capacity - 1);
        if (next == desired) return;
        map->entries[index].key = map->entries[next].key;
        map->entries[index].value = map->entries[next].value;
        index = next;
    }
}

void erase_entry(Hashmap *map, Value key) {
    if (map->count == 0) return;
    uint32_t index = get_hash(key) & (map->capacity - 1);

    while (true) {
        if (IS_EMPTY(map->entries[index])) {
            return;
        } else if (compare(map->entries[index].key, key)) {
            remove_by_index(map, index);
            map->count--;
            return;
        }
        index = (index + 1) & (map->capacity - 1);
    }
}

bool contains(Hashmap *map, Value key) {
    if (map->count == 0) return false;

    uint32_t hash = get_hash(key);
    uint64_t index = hash & (map->capacity - 1);
    uint64_t dist = 0;
    for (;;) {
        if (IS_EMPTY(map->entries[index])) return false;
        if (compare(map->entries[index].key, key)) return true;
        uint64_t desired = get_hash(map->entries[index].key) & (map->capacity - 1);
        uint64_t cur_dist = (index + map->capacity - desired) & (map->capacity - 1);
        if (cur_dist < dist) return false;
        dist++;
        index = (index + 1) & (map->capacity - 1);
    }
}

bool add_entry(Hashmap *map, Value key, Value value) {
    if (map->count + 1 > map->capacity * HASHMAP_MAX_LOAD) {
        grow_capacity(map);
    }

    uint32_t index = get_hash(key) & (map->capacity - 1);
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

        uint32_t desired = get_hash(map->entries[index].key) & (map->capacity - 1);
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


Entry *get_entry(Hashmap *map, Value key) {
    uint32_t hash = get_hash(key);
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

ObjString *get_string_entry(Hashmap *map, const char *key, int length, uint32_t hash) {
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

