#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include "vm.h"
#include "chunk.h"
#include "compiler.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

void initVM() {
    resetStack();
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
    } else if (IS_INTEGER(value)) {
        return value.as.integer != 0;
    } else if (IS_FLOAT(value)) {
        return value.as.floatingPoint != 0;
    }
    return true;
}

static void runtimeError(const char *format, ...) {
    va_list args;
    va_start(args, format);
    size_t instruction = vm.ip - vm.chunk->code - 1;
    int line = vm.chunk->lines[instruction];
    fprintf(stderr, "[line %d] RuntimeError: ", line);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    resetStack();
}

InterpretResult execute() {
#define READ_BYTE() (*vm.ip++)
#define NEXT_BYTE() (*(vm.ip))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
//TODO: Print compileError message when types are not a number
#define BINARY_NUMBER_OPERATION(castBool, operator, stringOperator) \
        do{ \
            if(!IS_NUMERIC(peek(0))  || !IS_NUMERIC(peek(1))){ \
                runtimeError("unsupported operand type(s) for %s: %s and %s.", stringOperator, typeToString(peek(1).type), typeToString(peek(0).type));                                    \
                return RUNTIME_ERROR; \
            }                                       \
                                                    \
            if(!IS_FLOAT(peek(0)) && !IS_FLOAT(peek(1))){      \
                int64_t b,a;                        \
                                                    \
                if(IS_BOOL(peek(0))){                   \
                    b = pop().as.boolean;\
                }else{                              \
                    b = pop().as.integer; \
                }                                   \
                                                    \
                if(IS_BOOL(peek(0))){                   \
                    a = pop().as.boolean;\
                }else{                              \
                    a = pop().as.integer; \
                }                                   \
                                                    \
                if(castBool)                                    \
                    push(BOOL_CAST((a operator b))); \
                else                                \
                    push(INTEGER_CAST(a operator b));\
            }else{                                  \
                double b,a;\
                if(IS_BOOL(peek(0))){                   \
                    b = pop().as.boolean;           \
                }else if(IS_INTEGER(peek(0))){       \
                    b = (double)pop().as.integer;           \
                }else{\
                    b = pop().as.floatingPoint; \
                }                                   \
                                                    \
                if(IS_BOOL(peek(0))){                   \
                    a = pop().as.boolean;           \
                }else if(IS_INTEGER(peek(0))){       \
                    a = (double)pop().as.integer;           \
                }else{\
                    a = pop().as.floatingPoint; \
                }                                      \
                                                    \
                if(castBool)                                    \
                    push(BOOL_CAST(a operator b)); \
                else                                \
                    push(FLOAT_CAST(a operator b));\
            }\
        }while(false)


#define BINARY_NUMBER_FUNCTION(castBool, function, stringFunction) \
        do{ \
            if(!IS_NUMERIC(peek(0))  || !IS_NUMERIC(peek(1))){ \
                runtimeError("unsupported operand type(s) for %s: %s and %s.", stringFunction, typeToString(peek(1).type), typeToString(peek(0).type));                                    \
                return RUNTIME_ERROR; \
            }                                       \
                                                    \
            if(!IS_FLOAT(peek(0)) && !IS_FLOAT(peek(1))){      \
                int64_t b,a;                        \
                                                    \
                if(IS_BOOL(peek(0))){                   \
                    b = pop().as.boolean;\
                }else{                              \
                    b = pop().as.integer; \
                }                                   \
                                                    \
                if(IS_BOOL(peek(0))){                   \
                    a = pop().as.boolean;\
                }else{                              \
                    a = pop().as.integer; \
                }                                   \
                                                    \
                if(castBool)                                    \
                    push(BOOL_CAST(function(a, b))); \
                else                                \
                    push(INTEGER_CAST(function(a, b)));\
            }else{                                  \
                double b,a;\
                if(IS_BOOL(peek(0))){                   \
                    b = pop().as.boolean;           \
                }else if(IS_INTEGER(peek(0))){       \
                    b = (double)pop().as.integer;           \
                }else{\
                    b = pop().as.floatingPoint; \
                }                                   \
                                                    \
                if(IS_BOOL(peek(0))){                   \
                    a = pop().as.boolean;           \
                }else if(IS_INTEGER(peek(0))){       \
                    a = (double)pop().as.integer;           \
                }else{\
                    a = pop().as.floatingPoint; \
                }                                      \
                                                    \
                if(castBool)                                    \
                    push(BOOL_CAST(function(a, b))); \
                else                                \
                    push(FLOAT_CAST(function(a, b)));\
            }\
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
                BINARY_NUMBER_OPERATION(false, +, "+");
                break;
            case OP_SUBTRACT:
                BINARY_NUMBER_OPERATION(false, -, "-");
                break;
            case OP_DIVIDE:
                if ((IS_INTEGER(peek(0)) && peek(0).as.integer == 0) ||
                    (IS_FLOAT(peek(0)) && peek(0).as.floatingPoint == 0.0) ||
                    (IS_BOOL(peek(0)) && peek(0).as.boolean == false)) {
                    runtimeError("division by zero.");
                    return RUNTIME_ERROR;
                }

                BINARY_NUMBER_OPERATION(false, /, "/");
                break;
            case OP_MULTIPLY:
                BINARY_NUMBER_OPERATION(false, *, "*");
                break;
            case OP_POW:
                BINARY_NUMBER_FUNCTION(false, pow, "^");
                break;
            case OP_PRINT:
                printValue(pop());
                printf("\n");
                break;
            case OP_GREATER:
                if (NEXT_BYTE() == OP_NOT) {
                    BINARY_NUMBER_OPERATION(true, >, "<=");
                } else {
                    BINARY_NUMBER_OPERATION(true, >, ">");
                }
                break;
            case OP_LESS:
                if (NEXT_BYTE() == OP_NOT) {
                    BINARY_NUMBER_OPERATION(true, >, "<=");
                } else {
                    BINARY_NUMBER_OPERATION(true, >, ">");
                }
                break;
            case OP_EQUALS:
                if (IS_FLOAT(peek(0)) && IS_FLOAT(peek(1))) {
                    push(BOOL_CAST(pop().as.floatingPoint == pop().as.floatingPoint));
                } else if (IS_INTEGER(peek(0)) && IS_INTEGER(peek(1))) {
                    push(BOOL_CAST(pop().as.integer == pop().as.integer));
                } else if (IS_BOOL(peek(0)) && IS_BOOL(peek(1))) {
                    push(BOOL_CAST(pop().as.boolean == pop().as.boolean));
                } else if (IS_NIL(peek(0)) && IS_NIL(peek(1))) {
                    push(BOOL_CAST(true));
                } else if (IS_FLOAT(peek(0)) && IS_INTEGER(peek(1))) {
                    push(BOOL_CAST(pop().as.floatingPoint == pop().as.integer));
                } else if (IS_INTEGER(peek(0)) && IS_FLOAT(peek(1))) {
                    push(BOOL_CAST(pop().as.integer == pop().as.floatingPoint));
                } else if (IS_BOOL(peek(0)) && IS_INTEGER(peek(1))) {
                    push(BOOL_CAST(pop().as.boolean == pop().as.integer));
                } else if (IS_INTEGER(peek(0)) && IS_BOOL(peek(1))) {
                    push(BOOL_CAST(pop().as.integer == pop().as.boolean));
                } else if (IS_BOOL(peek(0)) && IS_FLOAT(peek(1))) {
                    push(BOOL_CAST(pop().as.boolean == pop().as.floatingPoint));
                } else if (IS_FLOAT(peek(0)) && IS_BOOL(peek(1))) {
                    push(BOOL_CAST(pop().as.floatingPoint == pop().as.boolean));
                } else {
                    push(BOOL_CAST(false));
                }
                break;
            case OP_NEGATE:
                if (IS_INTEGER(peek(0))) {
                    push(INTEGER_CAST(-pop().as.integer));
                } else if (IS_FLOAT(peek(0))) {
                    push(FLOAT_CAST(-pop().as.floatingPoint));
                } else if (IS_BOOL(peek(0))) {
                    push(INTEGER_CAST(-pop().as.boolean));
                } else {
                    runtimeError("unsupported operand type for %s: %s.", "-", typeToString(peek(0).type));
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
#undef NEXT_BYTE
#undef BINARY_NUMBER_OPERATION
#undef BINARY_NUMBER_FUNCTION

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


