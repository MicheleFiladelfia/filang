#include "chunk.h"
#include "memory.h"



void initChunk(Chunk* chunk) {
    chunk->code = NULL;
    chunk->capacity = 0;
    chunk->count = 0;

    initValueArray(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(chunk->code, uint8_t, chunk->capacity);
    freeValueArray(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if(chunk->count + 1 >= chunk->capacity){
        int oldCapacity = chunk->capacity;
        chunk -> capacity = GROW_ARRAY_CAPACITY(chunk->capacity);
        chunk -> code = GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);
    }

    chunk->code[chunk->count] = byte;
    chunk->count++;
}

int writeConstant(Chunk* chunk, Value value){
    writeValueArray(&chunk->constants, value);
    return chunk->constants.count - 1;
}