#ifndef FILANG_MEMORY_H
#define FILANG_MEMORY_H

#include <stddef.h>
#include "value.h"

void *reallocate(void *pointer, size_t old_size, size_t new_size);

#define ALLOCATE(type, count) (type*)reallocate(NULL, 0, sizeof(type)*count)

#define GROW_ARRAY_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(array, type, old_count, new_count) \
    (type*)reallocate(array, sizeof(type) * (old_count), sizeof(type) * (new_count))

#define FREE_ARRAY(array, type, old_count) \
    reallocate(array, sizeof(type) * (old_count), 0)

#endif //FILANG_MEMORY_H
