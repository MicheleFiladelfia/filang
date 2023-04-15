#include "value.h"
#include "memory.h"
#include "hashmap.h"
#include "vm.h"
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
            printf(value.as.integer == 0 ? "false" : "true");
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
            if (IS_STRING(value))
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
            if (IS_STRING(value))
                return "<class 'String'>";
            else
                return "<class 'Object'>";
    }
}

static char *longToString(int64_t value) {
    char *buffer = ALLOCATE(char, 20);
    snprintf(buffer, 20, "%ld", value);
    return buffer;
}

static char *doubleToString(double value) {
    char *buffer = ALLOCATE(char, 20);
    snprintf(buffer, 20, "%.15g", value);
    return buffer;
}

ObjString *toString(Value value) {
    char *valueAsString;
    switch (value.type) {
        case VAL_INTEGER:
            valueAsString = longToString(value.as.integer);
            return makeObjString(valueAsString, (int) strlen(valueAsString));
        case VAL_FLOAT:
            valueAsString = doubleToString(value.as.floatingPoint);
            return makeObjString(valueAsString, (int) strlen(valueAsString));
        case VAL_BOOL:
            if (value.as.integer == 0) {
                return makeObjString("false", 6);
            } else {
                return makeObjString("true", 5);
            }
        case VAL_NIL:
            return makeObjString("nil", 4);
        case VAL_OBJECT:
            if (IS_STRING(value))
                return ((ObjString *) value.as.object);
            else
                return makeObjString(typeToString(value), (int) strlen(typeToString(value)));
    }
}

ObjString *makeObjString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = getStringEntry(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    ObjString *string = ALLOCATE(ObjString, 1);
    string->type = OBJ_STRING;
    string->length = length;
    string->chars = copyString(chars, length);
    string->hash = hash;

    addEntry(&vm.strings, string, NIL);
    return string;
}

ObjString *concatenateStrings(ObjString *a, ObjString *b) {
    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    return makeObjString(chars, length);
}