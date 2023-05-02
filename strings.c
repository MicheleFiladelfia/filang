#include <stdio.h>
#include <stddef.h>
#include "value.h"
#include "strings.h"
#include "memory.h"
#include "hashmap.h"
#include "vm.h"
#include <string.h>
#include <malloc.h>

char *type_to_string(Value value) {
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

static char *long_to_string(int64_t value) {
    char *buffer = ALLOCATE(char, 22);
    snprintf(buffer, 22, "%ld", value);
    return buffer;
}

static char *double_to_string(double value) {
    char *buffer = ALLOCATE(char, 22);
    snprintf(buffer, 22, "%.15g", value);
    return buffer;
}

ObjString *value_to_string(Value value) {
    char *value_as_string;
    switch (value.type) {
        case VAL_INTEGER:
            value_as_string = long_to_string(value.as.integer);
            return make_objstring(value_as_string, (int) strlen(value_as_string));
        case VAL_FLOAT:
            value_as_string = double_to_string(value.as.floatingPoint);
            return make_objstring(value_as_string, (int) strlen(value_as_string));
        case VAL_BOOL:
            if (value.as.integer == 0) {
                return make_objstring("false", 6);
            } else {
                return make_objstring("true", 5);
            }
        case VAL_OBJECT:
            if (IS_STRING(value))
                return ((ObjString *) value.as.object);
            else
                return make_objstring(type_to_string(value), (int) strlen(type_to_string(value)));
        case VAL_NIL:
        default:
            return make_objstring("nil", 4);
    }
}

ObjString *make_objstring(const char *chars, int length) {
    uint32_t hash = hash_string(chars, length);
    ObjString *interned = get_string_entry(&vm.strings, chars, length, hash);
    if (interned != NULL) return interned;

    ObjString *string = malloc(sizeof(ObjString) + sizeof(char) * (length + 1));
    string->type = OBJ_STRING;
    string->length = length;
    memcpy(string->chars, chars, length);
    string->chars[length] = '\0';
    string->hash = hash;

    add_entry(&vm.strings, STRING_CAST(string), NIL);
    return string;
}

ObjString *concatenate_strings(ObjString *a, ObjString *b) {
    int length = a->length + b->length;
    char *chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    return make_objstring(chars, length);
}
