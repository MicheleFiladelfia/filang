#include <stdio.h>
#include <stddef.h>
#include "value.h"
#include "strings.h"
#include "memory.h"
#include "hashmap.h"
#include "vm.h"
#include <string.h>
#include <malloc.h>

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
        default:
            return "<unknown type>";
    }
}

static char *longToString(int64_t value) {
    char *buffer = ALLOCATE(char, 22);
    snprintf(buffer, 22, "%ld", value);
    return buffer;
}

static char *doubleToString(double value) {
    char *buffer = ALLOCATE(char, 22);
    snprintf(buffer, 22, "%.15g", value);
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
        case VAL_OBJECT:
            if (IS_STRING(value))
                return ((ObjString *) value.as.object);
            else
                return makeObjString(typeToString(value), (int) strlen(typeToString(value)));
        case VAL_NIL:
        default:
            return makeObjString("nil", 4);
    }
}

ObjString *makeObjString(const char *chars, int length) {
    uint32_t hash = hashString(chars, length);
    ObjString *interned = getStringEntry(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    ObjString *string = malloc(sizeof(ObjString) + sizeof(char) * (length + 1));
    string->type = OBJ_STRING;
    string->length = length;
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    string->hash = hash;

    addEntry(&vm.strings, STRING_CAST(string), NIL);
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
