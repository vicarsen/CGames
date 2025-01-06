#pragma once

#include "type.h"

#include <stdio.h>
#include <stdlib.h>

#define ANSI_RED "\033[0;31m"
#define ANSI_GREEN "\033[0;32m"
#define ANSI_PURPLE "\033[0;35m"

#define INFO(...) logmsg(stdout, ANSI_GREEN, "info", __FILE__, __LINE__, __VA_ARGS__)
#define ERROR(...) logmsg(stderr, ANSI_RED, "error", __FILE__, __LINE__, __VA_ARGS__)

#define ASSERT(condition, ...) if (!(condition)) { logmsg(stderr, ANSI_PURPLE, "assert", __FILE__, __LINE__, __VA_ARGS__); exit(EXIT_FAILURE); }

void logmsg(FILE *out, char const *color, char const *severity, char const *file, i32 line, char const *msg, ...);

