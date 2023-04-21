#include <stdio.h>
#include <string.h>
#include "chunk.h"
#include "memory.h"


void initChunk(Chunk *chunk) {
    chunk->code = NULL;
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->lines->ends = NULL;
    chunk->lines->count = 0;
    chunk->lines->capacity = 0;

    initValueArray(&chunk->constants);
}

void freeChunk(Chunk *chunk) {
    FREE_ARRAY(chunk->code, uint8_t, chunk->capacity);
    FREE_ARRAY(chunk->lines->ends, int, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk *chunk, uint8_t byte, int line) {
    if (chunk->count + 1 >= chunk->capacity) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_ARRAY_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;

    if(line > chunk->lines->count){
         if (chunk->lines->count + 1 >= chunk->lines->capacity) {
            int oldCapacity = chunk->lines->capacity;
            chunk->lines->capacity = GROW_ARRAY_CAPACITY(chunk->lines->capacity);
            chunk->lines->ends = GROW_ARRAY(chunk->lines->ends, int, oldCapacity, chunk->lines->capacity);
            memset(&chunk->lines->ends[oldCapacity], 0, sizeof(int) * (chunk->lines->capacity - oldCapacity));
         }
         chunk->lines->count++;
    }

    // if the previous line is empty, set it to the same value as the line before
    if(chunk->lines->ends[line-2] == 0 && line > 1){
        chunk->lines->ends[line-2] = chunk->lines->ends[line-1];
    }

    chunk->lines->ends[line-1] = chunk->count;
    chunk->count++;
}

int writeConstant(Chunk *chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}