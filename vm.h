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
    Value *stack_top;
    Hashmap strings;
    Hashmap globals;
    struct {
        int size;
        int capacity;
        Value *local;
    } locals;
} VM;

extern VM vm;

void init_vm();

void free_vm();

InterpretResult interpret(const char *source);


#endif //FILANG_VM_H
