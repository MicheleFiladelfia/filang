#ifndef FILANG_CHUNK_H
#define FILANG_CHUNK_H

#include <stdint.h>
#include <stddef.h>
#include "value.h"


typedef enum {
    OP_ERROR,
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
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_CLOCK,
    OP_TYPEOF,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
} OpCode;

typedef struct {
    int count;
    int capacity;
    int *ends;
} Lines;

typedef struct {
    uint8_t *code;
    int count;
    int capacity;
    Lines lines;
    ValueArray constants;
} Chunk;

void init_chunk(Chunk *chunk);

void free_chunk(Chunk *chunk);

void write_chunk(Chunk *chunk, uint8_t byte, int line);

int write_constant(Chunk *chunk, Value value);

#endif //FILANG_CHUNK_H
