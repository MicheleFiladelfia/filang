#ifndef FILANG_STRINGS_H
#define FILANG_STRINGS_H

#include "value.h"

#define IS_STRING(value) (IS_OBJECT(value) && ((Object *) (value).as.object)->type == OBJ_STRING)
#define AS_STRING(value) ((ObjString *) AS_OBJECT(value))
#define STRING_CAST(value) ((Value){VAL_OBJECT, {.object = (Object *) (value)}})

typedef struct {
    Objtype type;
    int length;
    uint32_t hash;
    char chars[];
} ObjString;

char *typeToString(Value type);

ObjString *toString(Value value);

ObjString *makeObjString(const char *chars, int length);

ObjString *concatenateStrings(ObjString *a, ObjString *b);

#endif //FILANG_STRINGS_H
