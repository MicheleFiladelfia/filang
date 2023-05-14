#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "chunk.h"
#include "memory.h"


void init_chunk(Chunk *chunk) {
    chunk->code = NULL;
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->lines.ends = NULL;
    chunk->lines.capacity = 0;
    chunk->lines.count = 0;

    init_value_array(&chunk->constants);
}

void free_chunk(Chunk *chunk) {
    FREE_ARRAY(chunk->code, uint8_t, chunk->capacity);
    FREE_ARRAY(chunk->lines.ends, int, chunk->capacity);
    free_value_array(&chunk->constants);
    init_chunk(chunk);
}

void write_chunk(Chunk *chunk, uint8_t byte, int line) {
    if (chunk->count + 1 >= chunk->capacity) {
        int oldCapacity = chunk->capacity;
        chunk->capacity = GROW_ARRAY_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(chunk->code, uint8_t, oldCapacity, chunk->capacity);
        if (chunk->code == NULL) {
            fprintf(stderr, "Failed to allocate memory\n");
            exit(1);
        }
    }

    chunk->code[chunk->count] = byte;

    if (line > chunk->lines.count) {
        if (chunk->lines.count + 1 >= chunk->lines.capacity) {
            int oldCapacity = chunk->lines.capacity;
            chunk->lines.capacity = GROW_ARRAY_CAPACITY(chunk->lines.capacity);
            chunk->lines.ends = GROW_ARRAY(chunk->lines.ends, int, oldCapacity, chunk->lines.capacity);
            if (chunk->lines.ends == NULL) {
                fprintf(stderr, "Failed to allocate memory\n");
                exit(1);
            }
            memset(&chunk->lines.ends[oldCapacity], -1, sizeof(int) * (chunk->lines.capacity - oldCapacity));
        }
         chunk->lines.count++;
    }

    chunk->lines.ends[line-1] = chunk->count;
    chunk->count++;
}

int write_constant(Chunk *chunk, Value value) {
    write_value_array(&chunk->constants, value);
    return chunk->constants.count - 1;
}