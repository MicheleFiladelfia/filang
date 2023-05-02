#ifndef FILANG_HASHMAP_H
#define FILANG_HASHMAP_H

#include <stdint.h>
#include <stddef.h>
#include "value.h"
#include "value.h"
#include "strings.h"

/*
 * Robin Hood Hashmap implementation
 */

typedef struct {
    Value key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry *entries;
} Hashmap;

uint32_t hash_string(const char *key, int length);

void init_hashmap(Hashmap *map);

void free_hashmap(Hashmap *map);

bool add_entry(Hashmap *map, Value key, Value value);

Entry *get_entry(Hashmap *map, Value key);

void erase_entry(Hashmap *map, Value key);

bool contains(Hashmap *map, Value key);

ObjString *get_string_entry(Hashmap *map, const char *key, int length, uint32_t hash);

#define HASHMAP_MAX_LOAD 0.57
#define IS_EMPTY(entry) ((entry).key.type == VAL_NIL)


#endif //FILANG_HASHMAP_H
