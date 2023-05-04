#ifndef FILANG_VALUE_H
#define FILANG_VALUE_H


#include "token.h"
#include <stdbool.h>
#include <stdint.h>


typedef enum {
    TYPE_BOOL,
    TYPE_DECIMAL,
    TYPE_INTEGER,
    TYPE_OBJECT,
    TYPE_NIL
} ValueType;


typedef struct Object Object;

typedef struct {
    ValueType type;
    union {
        double decimal;
        int64_t integer;
        Object *object;
    } as;
} Value;

typedef struct {
    int count;
    int capacity;
    Value *values;
} ValueArray;

typedef enum {
    OBJ_STRING
} Objtype;

struct Object {
    Objtype type;
};


#define IS_OBJECT(value) ((value).type == TYPE_OBJECT)
#define IS_BOOL(value) ((value).type == TYPE_BOOL)
#define IS_FLOAT(value) ((value).type == TYPE_DECIMAL)
#define IS_INTEGER(value) (((value).type == TYPE_INTEGER) || IS_BOOL(value))
#define IS_NUMERIC(value) (IS_FLOAT(value) || IS_INTEGER(value) || IS_BOOL(value))
#define IS_NIL(value) ((value).type == VAL_NIL)

#define OBJECT_CAST(value) ((Value){TYPE_OBJECT, {.object = (Object *) (value)}})
#define BOOL_CAST(value) ((value) ? (Value){TYPE_BOOL, {.integer = true}} : (Value){TYPE_BOOL, {.integer = false}})
#define DECIMAL_CAST(value) ((Value){TYPE_DECIMAL, {.decimal = (double) value}})
#define INTEGER_CAST(value) ((Value){TYPE_INTEGER, {.integer = (int64_t) value}})
#define NIL ((Value){TYPE_NIL, {.integer = 0}})

#define AS_OBJECT(value) ((value).as.object)

void print_value(Value value);

void write_value_array(ValueArray *array, Value value);

void init_value_array(ValueArray *array);

void free_value_array(ValueArray *array);

#endif //FILANG_VALUE_H
