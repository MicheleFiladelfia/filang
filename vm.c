#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "strings.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

void initVM() {
    resetStack();
    initHashMap(&vm.strings);
}

void freeVM() {
    freeHashMap(&vm.strings);
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
    switch (value.type) {
        case VAL_BOOL:
            return value.as.integer != 0;
        case VAL_NIL:
            return false;
        case VAL_INTEGER:
            return value.as.integer != 0;
        case VAL_FLOAT:
            return value.as.floatingPoint != 0;
        case VAL_OBJECT:
            if (IS_STRING(value))
                return ((ObjString *) value.as.object)->length != 0;
            else
                return true;
        default:
            return true;
    }
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
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define BINARY_NUMBER_OPERATION(castBool, operator, stringOperator)                                                                       \
    do {                                                                                                                                  \
        if (!IS_NUMERIC(peek(0)) || !IS_NUMERIC(peek(1))) {                                                                               \
            runtimeError("unsupported operand type(s) for %s: %s and %s.", stringOperator, typeToString(peek(1)), typeToString(peek(0))); \
            return RUNTIME_ERROR;                                                                                                         \
        }                                                                                                                                 \
                                                                                                                                          \
        if (!IS_FLOAT(peek(0)) && !IS_FLOAT(peek(1)) && strcmp(stringOperator, "/") != 0) {                                               \
            int64_t b, a;                                                                                                                 \
                                                                                                                                          \
            b = pop().as.integer;                                                                                                         \
            a = pop().as.integer;                                                                                                         \
                                                                                                                                          \
            if (castBool)                                                                                                                 \
                push(BOOL_CAST((a operator b)));                                                                                          \
            else                                                                                                                          \
                push(INTEGER_CAST(a operator b));                                                                                         \
        }                                                                                                                                 \
        else {                                                                                                                            \
            double b, a;                                                                                                                  \
                                                                                                                                          \
            if (IS_INTEGER(peek(0))) {                                                                                                    \
                b = (double)pop().as.integer;                                                                                             \
            }                                                                                                                             \
            else {                                                                                                                        \
                b = pop().as.floatingPoint;                                                                                               \
            }                                                                                                                             \
                                                                                                                                          \
            if (IS_INTEGER(peek(0))) {                                                                                                    \
                a = (double)pop().as.integer;                                                                                             \
            }                                                                                                                             \
            else {                                                                                                                        \
                a = pop().as.floatingPoint;                                                                                               \
            }                                                                                                                             \
                                                                                                                                          \
            if (castBool)                                                                                                                 \
                push(BOOL_CAST(a operator b));                                                                                            \
            else                                                                                                                          \
                push(FLOAT_CAST(a operator b));                                                                                           \
        }                                                                                                                                 \
    } while (false)

#define BINARY_NUMBER_FUNCTION(castBool, function, stringFunction)                                                                        \
    do {                                                                                                                                  \
        if (!IS_NUMERIC(peek(0)) || !IS_NUMERIC(peek(1))) {                                                                               \
            runtimeError("unsupported operand type(s) for %s: %s and %s.", stringFunction, typeToString(peek(1)), typeToString(peek(0))); \
            return RUNTIME_ERROR;                                                                                                         \
        }                                                                                                                                 \
                                                                                                                                          \
        if (!IS_FLOAT(peek(0)) && !IS_FLOAT(peek(1))) {                                                                                   \
            int64_t b, a;                                                                                                                 \
                                                                                                                                          \
            b = pop().as.integer;                                                                                                         \
            a = pop().as.integer;                                                                                                         \
                                                                                                                                          \
            if (castBool)                                                                                                                 \
                push(BOOL_CAST(function(a, b)));                                                                                          \
            else                                                                                                                          \
                push(INTEGER_CAST(function(a, b)));                                                                                       \
        }                                                                                                                                 \
        else {                                                                                                                            \
            double b, a;                                                                                                                  \
                                                                                                                                          \
            if (IS_INTEGER(peek(0))) {                                                                                                    \
                b = (double)pop().as.integer;                                                                                             \
            }                                                                                                                             \
            else {                                                                                                                        \
                b = pop().as.floatingPoint;                                                                                               \
            }                                                                                                                             \
                                                                                                                                          \
            if (IS_INTEGER(peek(0))) {                                                                                                    \
                a = (double)pop().as.integer;                                                                                             \
            }                                                                                                                             \
            else {                                                                                                                        \
                a = pop().as.floatingPoint;                                                                                               \
            }                                                                                                                             \
                                                                                                                                          \
            if (castBool)                                                                                                                 \
                push(BOOL_CAST(function(a, b)));                                                                                          \
            else                                                                                                                          \
                push(FLOAT_CAST(function(a, b)));                                                                                         \
        }                                                                                                                                 \
    } while (false)

#define BINARY_INTEGER_OPERATION(operator, stringOperator)                                                                                \
    do {                                                                                                                                  \
        if ((!IS_BOOL(peek(0)) && !IS_INTEGER(peek(0))) || (!IS_BOOL(peek(1)) && !IS_INTEGER(peek(1)))) {                                 \
            runtimeError("unsupported operand type(s) for %s: %s and %s.", stringOperator, typeToString(peek(1)), typeToString(peek(0))); \
            return RUNTIME_ERROR;                                                                                                         \
        }                                                                                                                                 \
                                                                                                                                          \
        if (strcmp(stringOperator, "%") == 0) {                                                                                           \
            if (peek(0).as.integer == 0) {                                                                                                \
                runtimeError("division by zero.");                                                                                        \
                return RUNTIME_ERROR;                                                                                                     \
            }                                                                                                                             \
        }                                                                                                                                 \
                                                                                                                                          \
        int64_t b = pop().as.integer;                                                                                                     \
        int64_t a = pop().as.integer;                                                                                                     \
                                                                                                                                          \
        push(INTEGER_CAST(a operator b));                                                                                                 \
    } while (false)

    ObjString *name;
    for (;;) {
        switch (READ_BYTE()) {
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
                if (IS_STRING(peek(0)) || IS_STRING(peek(1))) {
                    push(OBJECT_CAST(concatenateStrings(toString(pop()), toString(pop()))));
                    break;
                }

                BINARY_NUMBER_OPERATION(false, +, "+");
                break;
            case OP_SUBTRACT:
                BINARY_NUMBER_OPERATION(false, -, "-");
                break;
            case OP_DIVIDE:
                if ((IS_INTEGER(peek(0)) && peek(0).as.integer == 0) ||
                    (IS_FLOAT(peek(0)) && peek(0).as.floatingPoint == 0.0)) {
                    runtimeError("division by zero.");
                    return RUNTIME_ERROR;
                }

                BINARY_NUMBER_OPERATION(false, /, "/");
                break;
            case OP_MULTIPLY:
                BINARY_NUMBER_OPERATION(false, *, "*");
                break;
            case OP_MODULO:
                BINARY_INTEGER_OPERATION(%, "%");
                break;
            case OP_POW:
                BINARY_NUMBER_FUNCTION(false, pow, "**");
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
                } else if (IS_NIL(peek(0)) && IS_NIL(peek(1))) {
                    push(BOOL_CAST(true));
                } else if (IS_FLOAT(peek(0)) && IS_INTEGER(peek(1))) {
                    push(BOOL_CAST(pop().as.floatingPoint == pop().as.integer));
                } else if (IS_INTEGER(peek(0)) && IS_FLOAT(peek(1))) {
                    push(BOOL_CAST(pop().as.integer == pop().as.floatingPoint));
                } else if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                    push(BOOL_CAST(AS_STRING(peek(0)) == AS_STRING(peek(1))));
                } else {
                    push(BOOL_CAST(false));
                }
                break;
            case OP_AND:
                if (!isTrue(peek(1)) || !isTrue(peek(0))) {
                    popValues(2);
                    push(BOOL_CAST(false));
                } else {
                    popValues(2);
                    push(BOOL_CAST(true));
                }
                break;
            case OP_OR:
                if (isTrue(peek(1)) || isTrue(peek(0))) {
                    popValues(2);
                    push(BOOL_CAST(true));
                } else {
                    popValues(2);
                    push(BOOL_CAST(false));
                }
                break;
            case OP_NEGATE:
                if (IS_INTEGER(peek(0))) {
                    push(INTEGER_CAST(-pop().as.integer));
                } else if (IS_FLOAT(peek(0))) {
                    push(FLOAT_CAST(-pop().as.floatingPoint));
                } else {
                    runtimeError("unsupported operand type for %s: %s.", "-", typeToString(peek(0)));
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
            case OP_BW_AND:
                BINARY_INTEGER_OPERATION(&, "&");
                break;
            case OP_BW_OR:
                BINARY_INTEGER_OPERATION(|, "|");
                break;
            case OP_XOR:
                BINARY_INTEGER_OPERATION(^, "^");
                break;
            case OP_BW_NOT:
                if (!IS_INTEGER(peek(0))) {
                    runtimeError("unsupported operand type for ~: %s.", typeToString(peek(0)));
                    return RUNTIME_ERROR;
                }

                push(INTEGER_CAST(~pop().as.integer));
                break;
            case OP_SHIFT_LEFT:
                BINARY_INTEGER_OPERATION(<<, "<<");
                break;
            case OP_SHIFT_RIGHT:
                BINARY_INTEGER_OPERATION(>>, ">>");
                break;
            case OP_POP:
                if (vm.repl) {
                    printValue(pop());
                    printf("\n");
                } else {
                    pop();
                }
                break;
            case OP_DEFINE_GLOBAL:
                name = READ_STRING();
                if (!contains(&vm.globals, name)) {
                    addEntry(&vm.globals, name, pop());
                } else {
                    runtimeError("redefinition of variable '%s'.", name->chars);
                    return RUNTIME_ERROR;
                }
                break;
            case OP_GET_GLOBAL:
                name = READ_STRING();
                if (contains(&vm.globals, name)) {
                    push(getEntry(&vm.globals, name));
                } else {
                    runtimeError("undefined variable: '%s'.", name->chars);
                    return RUNTIME_ERROR;
                }
                break;
            case OP_SET_GLOBAL:
                name = READ_STRING();
                if (contains(&vm.globals, name)) {
                    eraseEntry(&vm.globals, name);
                    addEntry(&vm.globals, name, peek(0));
                } else {
                    runtimeError("undefined variable: '%s'.", name->chars);
                    return RUNTIME_ERROR;
                }
                break;
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_STRING
#undef NEXT_BYTE
#undef BINARY_NUMBER_OPERATION
#undef BINARY_NUMBER_FUNCTION
#undef BINARY_INTEGER_OPERATION
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