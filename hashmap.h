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

void freeHashMap(HashMap *map);

bool addEntry(HashMap *map, ObjString *key, Value value);

Entry* getEntry(HashMap *map, ObjString *key);

void eraseEntry(HashMap *map, ObjString *key);

bool contains(HashMap *map, ObjString *key);

ObjString *getStringEntry(HashMap *map, const char *key, int length, uint32_t hash);

#define HASHMAP_MAX_LOAD 0.57
#define IS_EMPTY(entry) ((entry).key == NULL)


#endif //FILANG_HASHMAP_H
