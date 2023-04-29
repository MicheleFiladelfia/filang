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
} HashMap;

uint32_t hashString(const char *key, int length);

void initHashMap(HashMap *map);

void freeHashMap(HashMap *map);

bool addEntry(HashMap *map, Value key, Value value);

Entry *getEntry(HashMap *map, Value key);

void eraseEntry(HashMap *map, Value key);

bool contains(HashMap *map, Value key);

ObjString *getStringEntry(HashMap *map, const char *key, int length, uint32_t hash);

#define HASHMAP_MAX_LOAD 0.57
#define IS_EMPTY(entry) ((entry).key.type == VAL_NIL)


#endif //FILANG_HASHMAP_H
