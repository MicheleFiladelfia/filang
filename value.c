#include "value.h"
#include "memory.h"
#include "strings.h"
#include <stddef.h>
#include <stdio.h>

void init_value_array(ValueArray *array) {
    array->values = NULL;
    array->capacity = 0;
    array->count = 0;
}

void write_value_array(ValueArray *array, Value value) {
    if (array->count + 1 >= array->capacity) {
        int old_capacity = array->capacity;
        array->capacity = GROW_ARRAY_CAPACITY(array->capacity);
        array->values = GROW_ARRAY(array->values, Value, old_capacity, array->capacity);
    }

    array->values[(array->count)++] = value;
}

void free_value_array(ValueArray *array) {
    FREE_ARRAY(array->values, Value, array->capacity);
    init_value_array(array);
}

void print_value(Value value) {
    switch (value.type) {
        case TYPE_BOOL:
            printf(value.as.integer == 0 ? "false" : "true");
            break;
        case TYPE_DECIMAL:
            printf("%.15g", value.as.decimal);
            break;
        case TYPE_INTEGER:
            printf("%ld", value.as.integer);
            break;
        case TYPE_NIL:
            printf("nil");
            break;
        case TYPE_OBJECT:
            if (IS_STRING(value))
                printf("%s", ((ObjString *) value.as.object)->chars);
            else
                printf("%s", type_to_string(value));
            break;
    }
}

