#include "log.h"

#include <stdarg.h>
#include <stdio.h>

void logmsg(FILE *out, char const *color, char const *severity, char const *file, i32 line, char const *msg, ...)
{
  va_list arglist;
  va_start(arglist, msg);

  fprintf(out, "%s%s:%d (%s): ", color, file, line, severity);
  vfprintf(out, msg, arglist);
  fprintf(out, "\n");

  va_end(arglist);
}
