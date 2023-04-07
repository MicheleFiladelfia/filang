#include "value.h"
#include "memory.h"
#include <stddef.h>
#include <stdio.h>


void initValueArray(ValueArray *array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void writeValueArray(ValueArray *array, Value value) {
    if (array->count + 1 >= array->capacity) {
        int oldCapacity = array->capacity;
        array->capacity = GROW_ARRAY_CAPACITY(array->capacity);
        array->values = GROW_ARRAY(array->values, Value, oldCapacity, array->capacity);
    }

    array->values[(array->count)++] = value;
}

void freeValueArray(ValueArray *array) {
    FREE_ARRAY(array->values, Value, array->capacity);
    initValueArray(array);
}

void printValue(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(value.as.boolean == 0 ? "false" : "true");
            break;
        case VAL_FLOAT:
            printf("%.15g", value.as.floatingPoint);
            break;
        case VAL_INTEGER:
            printf("%ld", value.as.integer);
            break;
        case VAL_NIL:
            printf("nil");
            break;
    }
}

char *typeToString(ValueType type) {
    switch (type) {
        case VAL_BOOL:
            return "<primitive 'bool'>";
        case VAL_FLOAT:
            return "<primitive 'float'>";
        case VAL_INTEGER:
            return "<primitive 'integer'>";
        case VAL_NIL:
            return "<primitive 'nil'>";
    }
}
