#ifndef FILANG_CHUNK_H
#define FILANG_CHUNK_H

#include <stdint.h>
#include <stddef.h>
#include "value.h"


typedef enum {
    OP_RETURN,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MODULO,
    OP_NEGATE,
    OP_POW,
    OP_NOT,
    OP_AND,
    OP_OR,
    OP_BW_AND,
    OP_BW_OR,
    OP_XOR,
    OP_BW_NOT,
    OP_SHIFT_LEFT,
    OP_SHIFT_RIGHT,
    OP_TERNARY,
    OP_PRINT,
    OP_GREATER,
    OP_LESS,
    OP_EQUALS,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_CONSTANT_LONG_LONG,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
} OpCode;

typedef struct {
    uint8_t *code;
    int count;
    int capacity;
    int *lines;
    ValueArray constants;
} Chunk;

void initChunk(Chunk *chunk);

void freeChunk(Chunk *chunk);

void writeChunk(Chunk *chunk, uint8_t byte, int line);

int writeConstant(Chunk *chunk, Value value);

#endif //FILANG_CHUNK_H
