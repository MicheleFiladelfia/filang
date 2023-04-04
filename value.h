#ifndef FILANG_VALUE_H
#define FILANG_VALUE_H


#include "token.h"
#include <stdbool.h>

typedef enum{
    VAL_BOOL,
    VAL_NUMBER,
    VAL_NIL
} ValueType;

typedef struct{
    ValueType type;
    union{
        bool boolean;
        double number;
    } as;
} Value;

typedef struct{
    int count;
    int capacity;
    Value* values;
} ValueArray;


#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_NIL(value) ((value).type == VAL_NIL)
#define BOOL_CAST(value) ((value) ? (Value){VAL_BOOL, {.boolean = true}} : (Value){VAL_BOOL, {.boolean = false}})
#define NUMBER_CAST(value) ((Value){VAL_NUMBER, {.number = value}})
#define NIL ((Value){VAL_NIL, {.number = 0}})

void printValue(Value value);
void writeValueArray(ValueArray* array, Value value);
void initValueArray(ValueArray* array);
void freeValueArray(ValueArray* array);

#endif //FILANG_VALUE_H
