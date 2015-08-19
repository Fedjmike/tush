#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef int printf_t(const char*, ...);

/*=== Internal errors ====*/

extern _Atomic unsigned int internalerrors;

static inline void errprintf__ (const char* file, const char* fn, int line, const char* format, ...) {
    fprintf(stderr, "Gosh internal error: %s:%d: (%s): ", file, line, fn);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    internalerrors++;
}

typedef unsigned int errctx;

static inline errctx errcount (void) {
    return internalerrors;
}

static inline bool no_errors_recently (errctx errors) {
    return errors == internalerrors;
}

#define errprintf(...) errprintf__(__FILE__, __func__, __LINE__, __VA_ARGS__)

/*==== ====*/

typedef void* (*stdalloc)(size_t);

static inline void* malloci (size_t size, void* src) {
    void* obj = malloc(size);
    memcpy(obj, src, size);
    return obj;
}

/*strcat, resizing the buffer if need be*/
static inline char* strrecat (char* dest, size_t* size, const char* src) {
    size_t length = strlen(src) + strlen(dest) + 1;
    if (length > *size) {
        *size = length*2;
        dest = realloc(dest, *size);
    }

    return strcat(dest, src);
}
