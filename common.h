#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

typedef int printf_t(const char*, ...);

typedef void* (*stdalloc)(size_t);

static inline void* malloci (size_t size, void* src) {
    void* obj = malloc(size);
    memcpy(obj, src, size);
    return obj;
}

/*strcat, resizing the buffer if need be*/
inline char* strrecat (char* dest, size_t* size, const char* src) {
    size_t length = strlen(src) + strlen(dest) + 1;
    if (length > *size) {
        *size = length*2;
        dest = realloc(dest, *size);
    }

    return strcat(dest, src);
}
