#include <malloc.h>
#include <stdlib.h>
#include "memory.h"

void *reallocate(void *pointer, size_t oldSize, size_t newSize) {
    if (newSize == 0) {
        free(pointer);
        return NULL;
    }

    void *allocatedMemory = realloc(pointer, newSize);
    if (allocatedMemory == NULL) {
        exit(1);
    }

    return allocatedMemory;
}
