#ifndef FILANG_MEMORY_H
#define FILANG_MEMORY_H

#include <stddef.h>

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#define allocate(type,count) (type*)reallocate(NULL, 0, sizeof(type)*count)

#define GROW_ARRAY_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(array, type, oldCount, newCount) \
    (type*)reallocate(array, sizeof(type) * (oldCount), sizeof(type) * (newCount))

#define FREE_ARRAY(array, type, oldCount) \
    reallocate(array, sizeof(type) * (oldCount), 0)

#define FREE(array, type, oldCount) \
    reallocate(array, sizeof(type), 0)

#endif //FILANG_MEMORY_H
