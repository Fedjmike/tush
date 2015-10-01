#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <gc.h>

typedef int printf_t(const char*, ...);

/*==== Internal errors ====*/

extern _Atomic unsigned int internalerrors;

static inline void errprintf__ (const char* file, const char* fn, int line, const char* format, ...) {
    fprintf(stderr, "Tush internal error: %s:%d: (%s): ", file, line, fn);
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

#define precond(cond) \
    ((cond) ? true : (errprintf("Precondition '%s' is false\n", #cond), false))

/*==== ====*/

static inline void* GC_calloc_(size_t n, size_t size) {
    size_t total = n*size;

    if (!precond(total/n == size))
        return 0;

    return GC_malloc(total);
};

/*GC_malloc does clearing for you*/
#define gcalloc ((alloc_t) {GC_malloc, GC_calloc_, GC_free, GC_realloc, GC_strdup})
