#ifndef FILANG_VALUE_H
#define FILANG_VALUE_H


#include "token.h"
#include <stdbool.h>
#include <stdint-gcc.h>

typedef enum {
    VAL_BOOL,
    VAL_FLOAT,
    VAL_INTEGER,
    VAL_NIL
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double floatingPoint;
        int64_t integer;
    } as;
} Value;

typedef struct {
    int count;
    int capacity;
    Value *values;
} ValueArray;


#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_FLOAT(value) ((value).type == VAL_FLOAT)
#define IS_INTEGER(value) ((value).type == VAL_INTEGER)
#define IS_NUMERIC(value) (IS_FLOAT(value) || IS_INTEGER(value) || IS_BOOL(value))
#define IS_NIL(value) ((value).type == VAL_NIL)
#define BOOL_CAST(value) ((value) ? (Value){VAL_BOOL, {.boolean = true}} : (Value){VAL_BOOL, {.boolean = false}})
#define FLOAT_CAST(value) ((Value){VAL_FLOAT, {.floatingPoint = (double) value}})
#define INTEGER_CAST(value) ((Value){VAL_INTEGER, {.integer = (int64_t) value}})
#define NIL ((Value){VAL_NIL, {.integer = 0}})

void printValue(Value value);

void writeValueArray(ValueArray *array, Value value);

void initValueArray(ValueArray *array);

void freeValueArray(ValueArray *array);

#endif //FILANG_VALUE_H
