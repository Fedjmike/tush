#include "terminal.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>

unsigned int getWindowWidth (void) {
    struct winsize size;
    ioctl(0, TIOCGWINSZ, &size);

    return size.ws_col;
}

static int vfprintf_style (FILE* file, const char* roformat, va_list args) {
    int printed = 0;

    size_t formatlen = strlen(roformat);

    /*Make a writable copy of the format. Note: VLA*/
    char wformat[formatlen+1];
    strcpy(wformat, roformat);

    char* format = wformat;

    while (*format) {
        /*Find the next opening brace and print up until it
          (or to the end of the string, if none)*/

        char* lbrace = strchr(format, '{');

        if (lbrace)
            *lbrace = 0;

        printed += fprintf(file, format, args);

        if (!lbrace)
            break;

        /*Apply the style*/

        const char* style = va_arg(args, const char*);
        fprintf(file, "%s", style);

        /*Print up until the closing brace*/

        char* rbrace = strchr(lbrace+1, '}');

        if (rbrace)
            *rbrace = 0;

        printed += vfprintf(file, lbrace+1, args);

        /*Move on*/

        if (!rbrace)
            break;

        else
            format = rbrace+1;

        fprintf(file, "%s", styleReset);
    }

    return printed;
}

int fprintf_style (FILE* file, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int printed = vfprintf_style(file, format, args);
    va_end(args);

    return printed;
}

int printf_style (const char* format, ...) {
    va_list args;
    va_start(args, format);
    int printed = vfprintf_style(stdout, format, args);
    va_end(args);

    return printed;
}
