#include "strings.h"
#include "entities.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

char *formatted_string(const char *format, ...) {
  va_list arg;
  va_start(arg, format);
  size_t size = vsnprintf(NULL, 0, format, arg) + 1;
  va_end(arg);
  char *buf = (char *)malloc(size);
  if (buf == NULL) {
    perror("Could not allocate memory for formatted string");
    return NULL;
  }
  va_start(arg, format);
  vsprintf(buf, format, arg);
  buf[size - 1] = '\0';
  va_end(arg);
  return buf;
}

void log_error(simulation_t *sim, int result, const char *cause) {
  const char *error = GRBgeterrormsg(sim->env);
  perror(formatted_string("Error %s (code: %d) caused by: %s", error, result,
                          cause));
}
