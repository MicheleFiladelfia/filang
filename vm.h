#ifndef FILANG_VM_H
#define FILANG_VM_H

#include <stdint.h>
#include "chunk.h"

typedef enum {
    NO_ERRORS,
    COMPILE_ERROR,
    RUNTIME_ERROR
} InterpretResult;


typedef struct {
    Chunk *chunk;
    uint8_t *ip;
    Value stack[256];
    Value *stackTop;
} VM;


void initVM();

void freeVM();

void push(Value value);

Value pop();

InterpretResult interpret(const char *source);


#endif //FILANG_VM_H
