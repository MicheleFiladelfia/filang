#ifndef FILANG_HASHMAP_H
#define FILANG_HASHMAP_H

#include <stdint-gcc.h>
#include <stddef.h>
#include "value.h"
#include "value.h"

/*
 * Robin Hood Hashmap implementation
 */

typedef struct {
    ObjString *key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry *entries;
} HashMap;

uint32_t hashString(const char *key, int length);

void initHashMap(HashMap *map);

void addEntry(HashMap *map, ObjString *key, Value value);

Value getEntry(HashMap *map, ObjString *key);

void erase(HashMap *map, ObjString *key);

void freeHashMap(HashMap *map);

ObjString *getStringEntry(HashMap *map, const char *key, int length, uint32_t hash);

#define HASHMAP_MAX_LOAD 0.53
#define IS_EMPTY(entry) ((entry).key == NULL)


#endif //FILANG_HASHMAP_H
