#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*This must be given storage (and a value) by the test itself,
  using TEST_GLOBAL_SETUP, which also provides main() and
  internalerrors.*/
extern _Atomic unsigned int test_errors;

/*test_main must be the name of a function,
    void ()(void)
  which runs the tests.*/
#define TEST_GLOBAL_SETUP(test_main)          \
    _Atomic unsigned int internalerrors = 0;  \
    _Atomic unsigned int test_errors = 0;     \
    int main (int argc, char** argv) {        \
        (void) argc, (void) argv;             \
        test_main();                          \
        return test_errors;                   \
    }

static inline void test_errprintf (const char* file, const char* fn, int line, const char* format, ...) {
    (void) fn;
    fprintf(stderr, "test failed: %s:%d: ", file, line);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    test_errors++;
}

static inline void require_ (bool success, const char* test_str,
                             const char* file, const char* fn, int line) {
    if (!success) {
        test_errprintf(file, fn, line, "required %s\n", test_str);
        exit(test_errors);
    }
}

#define require(expr) \
    require_((expr), #expr, __FILE__, __func__, __LINE__)

static inline void expect_ (bool success, const char* test_str,
                            const char* file, const char* fn, int line) {
    if (!success)
        test_errprintf(file, fn, line, "expected %s\n", test_str);
}

#define expect(expr) \
    expect_((expr), #expr, __FILE__, __func__, __LINE__)

static inline void expect_null_ (void* ptr, const char* test_str,
                                 const char* file, const char* fn, int line) {
    if (ptr != 0)
        test_errprintf(file, fn, line, "expected %s (%p) to be null\n", test_str, ptr);
}

#define expect_null(expr) \
    expect_null_((expr), #expr, __FILE__, __func__, __LINE__)

static inline void expect_equal_ (intmax_t l, intmax_t r, const char* l_str, const char* r_str,
                                  const char* file, const char* fn, int line) {
    if (l != r)
        test_errprintf(file, fn, line, "expected %s (%d) and %s (%d) to be equal\n", l_str, l, r_str, r);
}

#define expect_equal(l, r) \
    expect_equal_((intmax_t)(l), (intmax_t)(r), #l, #r, __FILE__, __func__, __LINE__)

static inline void expect_str_equal_ (const char* expected, const char* str, const char* str_str,
                                      const char* file, const char* fn, int line) {
    if (!str) {
        test_errprintf(file, fn, line, "expected %s (%p) to be '%s', null given\n",
                       str_str, str, expected);

    } else if (!!strcmp(expected, str))
        test_errprintf(file, fn, line, "expected %s (%p, '%s') to be '%s'\n",
                       str_str, str, str, expected);
}

#define expect_str_equal(expected, expr) \
    expect_str_equal_((expected), (expr), #expr, __FILE__, __func__, __LINE__)
