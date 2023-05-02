#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "strings.h"

VM vm;

static void reset_stack() {
    vm.stackTop = vm.stack;
}

void init_vm() {
    reset_stack();
    init_hashmap(&vm.strings);
}

void free_vm() {
    free_hashmap(&vm.strings);
    free_hashmap(&vm.globals);
}

static void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

static Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

static void pop_n(int n) {
    vm.stackTop -= n;
}

static Value peek(int count) {
    return vm.stackTop[-1 - count];
}

static Value *peek_pointer(int count) {
    return vm.stackTop - 1 - count;
}

static bool is_true(Value value) {
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

static void runtime_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    size_t instruction = vm.ip - vm.chunk->code - 1;

    int line = 1;
    for (int i = 0; i < vm.chunk->lines.count; i++) {
        if ((size_t) vm.chunk->lines.ends[i] > instruction && vm.chunk->lines.ends[i] != -1) {
            line = i + 1;
            break;
        }
    }

    fprintf(stderr, "[line %d] RuntimeError: ", line);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
    reset_stack();
}

InterpretResult execute() {
#define READ_BYTE() (*(vm.ip++))
#define NEXT_BYTE() (*(vm.ip))
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_BYTE() + (READ_BYTE()<<8)])
#define READ_CONSTANT_LONG_LONG() (vm.chunk->constants.values[READ_BYTE() + (READ_BYTE()<<8) + (READ_BYTE()<<16)])
#define READ_STRING_BYTE() AS_STRING(READ_CONSTANT())
#define READ_STRING(str)                     \
    switch (READ_BYTE()) {                   \
    case OP_CONSTANT:                        \
        str = READ_CONSTANT();               \
        break;                               \
    case OP_CONSTANT_LONG:                   \
        str = READ_CONSTANT_LONG();          \
        break;                               \
    case OP_CONSTANT_LONG_LONG:              \
        str = READ_CONSTANT_LONG_LONG();     \
        break;                               \
    default:                                 \
        runtime_error("An error occurred."); \
        return RUNTIME_ERROR;                \
    }

#define BINARY_NUMBER_OPERATION(castBool, operator, string_operator)                                                                            \
    do {                                                                                                                                        \
        if (!IS_NUMERIC(peek(0)) || !IS_NUMERIC(peek(1))) {                                                                                     \
            runtime_error("unsupported operand type(s) for %s: %s and %s.", string_operator, type_to_string(peek(1)), type_to_string(peek(0))); \
            return RUNTIME_ERROR;                                                                                                               \
        }                                                                                                                                       \
                                                                                                                                                \
        if (!IS_FLOAT(peek(0)) && !IS_FLOAT(peek(1)) && strcmp(string_operator, "/") != 0) {                                                    \
            int64_t b, a;                                                                                                                       \
                                                                                                                                                \
            b = pop().as.integer;                                                                                                               \
            a = pop().as.integer;                                                                                                               \
                                                                                                                                                \
            if (castBool)                                                                                                                       \
                push(BOOL_CAST((a operator b)));                                                                                                \
            else                                                                                                                                \
                push(INTEGER_CAST(a operator b));                                                                                               \
        }                                                                                                                                       \
        else {                                                                                                                                  \
            double b, a;                                                                                                                        \
                                                                                                                                                \
            if (IS_INTEGER(peek(0))) {                                                                                                          \
                b = (double)pop().as.integer;                                                                                                   \
            }                                                                                                                                   \
            else {                                                                                                                              \
                b = pop().as.floatingPoint;                                                                                                     \
            }                                                                                                                                   \
                                                                                                                                                \
            if (IS_INTEGER(peek(0))) {                                                                                                          \
                a = (double)pop().as.integer;                                                                                                   \
            }                                                                                                                                   \
            else {                                                                                                                              \
                a = pop().as.floatingPoint;                                                                                                     \
            }                                                                                                                                   \
                                                                                                                                                \
            if (castBool)                                                                                                                       \
                push(BOOL_CAST(a operator b));                                                                                                  \
            else                                                                                                                                \
                push(FLOAT_CAST(a operator b));                                                                                                 \
        }                                                                                                                                       \
    } while (false)

#define BINARY_INTEGER_OPERATION(operator, string_operator)                                                                                     \
    do {                                                                                                                                        \
        if ((!IS_BOOL(peek(0)) && !IS_INTEGER(peek(0))) || (!IS_BOOL(peek(1)) && !IS_INTEGER(peek(1)))) {                                       \
            runtime_error("unsupported operand type(s) for %s: %s and %s.", string_operator, type_to_string(peek(1)), type_to_string(peek(0))); \
            return RUNTIME_ERROR;                                                                                                               \
        }                                                                                                                                       \
                                                                                                                                                \
        if (strcmp(string_operator, "%") == 0 && peek(0).as.integer == 0) {                                                                     \
            runtime_error("division by zero.");                                                                                                 \
            return RUNTIME_ERROR;                                                                                                               \
        }                                                                                                                                       \
                                                                                                                                                \
        int64_t b = pop().as.integer;                                                                                                           \
        int64_t a = pop().as.integer;                                                                                                           \
                                                                                                                                                \
        push(INTEGER_CAST(a operator b));                                                                                                       \
    } while (false)

    Value temp;
    Entry *entry;
    char *cstr;

    for (;;) {
        switch (READ_BYTE()) {
            case OP_RETURN:
                return NO_ERRORS;
            case OP_CONSTANT:
                push(READ_CONSTANT());
                break;
            case OP_CONSTANT_LONG:
                push(READ_CONSTANT_LONG());
                break;
            case OP_CONSTANT_LONG_LONG:
                push(READ_CONSTANT_LONG_LONG());
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
                    push(OBJECT_CAST(concatenate_strings(value_to_string(pop()), value_to_string(pop()))));
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
                    runtime_error("division by zero.");
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
                if (!IS_NUMERIC(peek(0)) || !IS_NUMERIC(peek(1))) {
                    runtime_error("unsupported operand type(s) for '**': %s and %s.", type_to_string(peek(1)),
                                  type_to_string(peek(0)));
                    return RUNTIME_ERROR;
                }

                double b, a;

                if (IS_INTEGER(peek(0))) {
                    b = (double) pop().as.integer;
                } else {
                    b = pop().as.floatingPoint;
                }

                if (IS_INTEGER(peek(0))) {
                    a = (double) pop().as.integer;
                } else {
                    a = pop().as.floatingPoint;
                }

                double result = pow(a, b);

                if (floor(result) == result) {
                    push(INTEGER_CAST((int64_t) result));
                } else {
                    push(FLOAT_CAST(result));
                }
                break;
            case OP_PRINT:
                print_value(pop());
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
                switch (peek(0).type) {
                    case VAL_BOOL:
                    case VAL_INTEGER:
                        switch (peek(1).type) {
                            case VAL_BOOL:
                            case VAL_INTEGER:
                                push(BOOL_CAST(pop().as.integer == pop().as.integer));
                                break;
                            case VAL_FLOAT:
                                push(BOOL_CAST(pop().as.integer == pop().as.floatingPoint));
                                break;
                            case VAL_OBJECT:
                            case VAL_NIL:
                                push(BOOL_CAST(false));
                                break;
                        }
                        break;
                    case VAL_FLOAT:
                        switch (peek(1).type) {
                            case VAL_BOOL:
                            case VAL_INTEGER:
                                push(BOOL_CAST(pop().as.floatingPoint == pop().as.integer));
                                break;
                            case VAL_FLOAT:
                                push(BOOL_CAST(pop().as.floatingPoint == pop().as.floatingPoint));
                                break;
                            case VAL_OBJECT:
                            case VAL_NIL:
                                push(BOOL_CAST(false));
                                break;
                        }
                        break;
                    case VAL_OBJECT:
                        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
                            push(BOOL_CAST(AS_STRING(peek(0)) == AS_STRING(peek(1))));
                        } else {
                            push(BOOL_CAST(false));
                        }
                        break;
                    case VAL_NIL:
                        push(BOOL_CAST(peek(1).type == VAL_NIL));
                        break;
                }
                break;
            case OP_AND:
                if (!is_true(peek(1)) || !is_true(peek(0))) {
                    pop_n(2);
                    push(BOOL_CAST(false));
                } else {
                    pop_n(2);
                    push(BOOL_CAST(true));
                }
                break;
            case OP_OR:
                if (is_true(peek(1)) || is_true(peek(0))) {
                    pop_n(2);
                    push(BOOL_CAST(true));
                } else {
                    pop_n(2);
                    push(BOOL_CAST(false));
                }
                break;
            case OP_NEGATE:
                if (IS_INTEGER(peek(0))) {
                    *peek_pointer(0) = INTEGER_CAST(-peek(0).as.integer);
                } else if (IS_FLOAT(peek(0))) {
                    *peek_pointer(0) = FLOAT_CAST(-peek(0).as.floatingPoint);
                } else {
                    runtime_error("unsupported operand type for %s: %s.", "-", type_to_string(peek(0)));
                    return RUNTIME_ERROR;
                }
                break;
            case OP_NOT:
                *peek_pointer(0) = BOOL_CAST(!is_true(peek(0)));
                break;
            case OP_TERNARY:
                if (is_true(peek(2))) {
                    temp = peek(1);
                    pop_n(3);
                    push(temp);
                } else {
                    temp = peek(0);
                    pop_n(3);
                    push(temp);
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
                    runtime_error("unsupported operand type for ~: %s.", type_to_string(peek(0)));
                    return RUNTIME_ERROR;
                }

                *peek_pointer(0) = INTEGER_CAST(~peek(0).as.integer);
                break;
            case OP_SHIFT_LEFT:
                BINARY_INTEGER_OPERATION(<<, "<<");
                break;
            case OP_SHIFT_RIGHT:
                BINARY_INTEGER_OPERATION(>>, ">>");
                break;
            case OP_POP:
                if (vm.repl) {
                    print_value(pop());
                    printf("\n");
                } else {
                    pop();
                }
                break;
            case OP_DEFINE_GLOBAL:
                READ_STRING(temp);

                if (add_entry(&vm.globals, temp, pop())) {
                    runtime_error("redefinition of variable '%s'.", AS_STRING(temp)->chars);
                    return RUNTIME_ERROR;
                }

                break;
            case OP_GET_GLOBAL:
                READ_STRING(temp);

                entry = get_entry(&vm.globals, temp);

                if (entry != NULL) {
                    push(entry->value);
                } else {
                    runtime_error("undefined variable: '%s'.", AS_STRING(temp)->chars);
                    return RUNTIME_ERROR;
                }

                break;
            case OP_SET_GLOBAL:
                READ_STRING(temp);

                entry = get_entry(&vm.globals, temp);

                if (entry != NULL) {
                    entry->value = peek(0);
                    entry->key = temp;
                } else {
                    runtime_error("undefined variable: '%s'.", AS_STRING(temp)->chars);
                    return RUNTIME_ERROR;
                }

                break;
            case OP_CLOCK:
                push(FLOAT_CAST((double) clock() / CLOCKS_PER_SEC));
                break;
            case OP_TYPEOF:
                cstr = type_to_string(pop());
                push(OBJECT_CAST(make_objstring(cstr, strlen(cstr))));
                break;
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_LONG
#undef READ_CONSTANT_LONG_LONG
#undef READ_STRING_BYTE
#undef READ_STRING
#undef NEXT_BYTE
#undef BINARY_NUMBER_OPERATION
#undef BINARY_NUMBER_FUNCTION
#undef BINARY_INTEGER_OPERATION
}

InterpretResult interpret(const char *source) {
    Chunk chunk;
    init_chunk(&chunk);

    if (!compile(&chunk, source)) {
        free_chunk(&chunk);
        return COMPILE_ERROR;
    }

    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult result = execute();

    free_chunk(&chunk);
    return result;
}