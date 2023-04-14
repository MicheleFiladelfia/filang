#ifndef FILANG_VM_H
#define FILANG_VM_H

#include <stdint.h>
#include "chunk.h"
#include "hashmap.h"

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
    HashMap strings;
    HashMap globals;
} VM;

extern VM vm;

void initVM();

void freeVM();

InterpretResult interpret(const char *source);


#endif //FILANG_VM_H
