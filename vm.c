#include <stdio.h>
#include <math.h>
#include "vm.h"
#include "chunk.h"
#include "compiler.h"

VM vm;

void initVM() {
    vm.stackTop = vm.stack;
}

void freeVM() {

}

static void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

static Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static void popValues(int n) {
    vm.stackTop -= n;
}

static Value peek(int count) {
    return vm.stackTop[-1 - count];
}

static bool isTrue(Value value) {
    if (IS_BOOL(value)) {
        return value.as.boolean;
    } else if (IS_NIL(value)) {
        return false;
    } else if (IS_NUMBER(value)) {
        return value.as.number != 0;
    }

    return true;
}

InterpretResult execute() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
//TODO: Print error message when types are not a number
#define BINARY_NUMBER_OPERATION(castType, operator) \
        do{ \
            if((!IS_NUMBER(peek(0)) && !IS_BOOL(peek(0))) || (!IS_NUMBER(peek(1)) && !IS_BOOL(peek(1)))){ \
                return RUNTIME_ERROR; \
            }                                       \
            double b,a;\
            if(IS_BOOL(peek(0))){                   \
                b = pop().as.boolean;\
            }else{\
                b = pop().as.number; \
            }\
    \
            if(IS_BOOL(peek(0))){                   \
                a = pop().as.boolean;\
            }else{\
                a = pop().as.number; \
            }\
                                                    \
            push(castType((a operator b)));                  \
        }while(false)


#define BINARY_NUMBER_FUNCTION(castType, function) \
        do{ \
            if((!IS_NUMBER(peek(0)) && !IS_BOOL(peek(0))) || (!IS_NUMBER(peek(1)) && !IS_BOOL(peek(1)))){ \
                return RUNTIME_ERROR; \
            }                                       \
            double b,a;\
            if(IS_BOOL(peek(0))){                   \
                b = pop().as.boolean;\
            }else{\
                b = pop().as.number; \
            }\
    \
            if(IS_BOOL(peek(0))){                   \
                a = pop().as.boolean;\
            }else{\
                a = pop().as.number; \
            }\
                                                    \
            push(castType(function(a,b)));                  \
        }while(false)


    for (;;) {
        uint8_t instruction;

        switch (instruction = READ_BYTE()) {
            case OP_RETURN:
                return NO_ERRORS;
            case OP_CONSTANT:
                push(READ_CONSTANT());
                break;
            case OP_TRUE:
                push(BOOL_CAST(true));
                break;
            case OP_FALSE:
                push(BOOL_CAST(false));
                break;
            case OP_NIL:
                push(NIL);
                break;
            case OP_ADD:
                BINARY_NUMBER_OPERATION(NUMBER_CAST, +);
                break;
            case OP_SUBTRACT:
                BINARY_NUMBER_OPERATION(NUMBER_CAST, -);
                break;
            case OP_DIVIDE:
                BINARY_NUMBER_OPERATION(NUMBER_CAST, /);
                break;
            case OP_MULTIPLY:
                BINARY_NUMBER_OPERATION(NUMBER_CAST, *);
                break;
            case OP_POW:
                BINARY_NUMBER_FUNCTION(NUMBER_CAST, pow);
                break;
            case OP_PRINT:
                printValue(pop());
                printf("\n");
                break;
            case OP_GREATER:
                BINARY_NUMBER_OPERATION(BOOL_CAST, >);
                break;
            case OP_LESS:
                BINARY_NUMBER_OPERATION(BOOL_CAST, <);
                break;
            case OP_EQUALS:
                if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
                    push(BOOL_CAST(pop().as.number == pop().as.number));
                } else if (IS_BOOL(peek(0)) && IS_BOOL(peek(1))) {
                    push(BOOL_CAST(pop().as.boolean == pop().as.boolean));
                } else if (IS_NIL(peek(0)) && IS_NIL(peek(1))) {
                    push(BOOL_CAST(true));
                } else {
                    push(BOOL_CAST(false));
                }
                break;
            case OP_NEGATE:
                if (IS_NUMBER(peek(0))) {
                    push(NUMBER_CAST(-pop().as.number));
                } else {
                    //TODO: Print error message
                    return RUNTIME_ERROR;
                }
                break;
            case OP_NOT:
                push(BOOL_CAST(!isTrue(pop())));
                break;
            case OP_TERNARY:
                if (isTrue(peek(2))) {
                    Value result = peek(1);
                    popValues(3);
                    push(result);
                } else {
                    Value result = peek(0);
                    popValues(3);
                    push(result);
                }
                break;
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef BINARY_NUMBER_OPERATION

}

InterpretResult interpret(const char *source) {
    Chunk chunk;
    initChunk(&chunk);

    if (!compile(&chunk, source)) {
        freeChunk(&chunk);
        return COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = execute();
    freeChunk(&chunk);
    return result;
}


