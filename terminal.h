#pragma once

#include "stdarg.h"
#include "stdio.h"

#define styleBlack   "\e[1;30m"
#define styleRed     "\e[1;31m"
#define styleGreen   "\e[1;32m"
#define styleYellow  "\e[1;33m"
#define styleBlue    "\e[1;34m"
#define styleMagenta "\e[1;35m"
#define styleCyan    "\e[1;36m"
#define styleWhite   "\e[1;37m"
#define styleReset   "\e[0m"

int printf_style (const char* format, ...);
int fprintf_style (FILE* file, const char* format, ...);
