#pragma once

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
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
