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
    bool repl;
    Chunk *chunk;
    uint8_t *ip;
    Value stack[256];
    Value *stackTop;
    Hashmap strings;
    Hashmap globals;
} VM;

extern VM vm;

void init_vm();

void free_vm();

InterpretResult interpret(const char *source);


#endif //FILANG_VM_H
