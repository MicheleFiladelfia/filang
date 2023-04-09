#include "value.h"
#include "memory.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>


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
        case VAL_OBJECT:
            if(IS_STRING(value))
                printf("%s", ((ObjString *) value.as.object)->chars);
            else
                printf("%s", typeToString(value));
            break;
    }
}

char *typeToString(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            return "<builtin 'bool'>";
        case VAL_FLOAT:
            return "<builtin 'float'>";
        case VAL_INTEGER:
            return "<builtin 'integer'>";
        case VAL_NIL:
            return "<builtin 'nil'>";
        case VAL_OBJECT:
            if(IS_STRING(value))
                return "<class 'String'>";
            else
                return "<class 'Object'>";
    }
}


ObjString* makeObjString(char* chars, int length) {
    ObjString* string = ALLOCATE(ObjString, 1);
    string->type = OBJ_STRING;
    string->length = length;
    string->chars = copyString(chars, length);
    return string;
}

ObjString* concatenateStrings(ObjString* a, ObjString* b) {
    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    return makeObjString(chars, length);
}