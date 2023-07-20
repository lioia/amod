#include "utils.h"
#include "entities.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#elif _WIN32
#define SetWindowText SetWindowTextA
#include <windows.h>
#endif

void quicksort(instance_t *instance, int low, int high);
int partition(instance_t *instance, int low, int high);
void swap(int *a, int *b);

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

int create_folder(const char *path) {
  int result = 0;
#ifdef __linux__
  struct stat st = {0};
  if (stat(path, &st) == -1)
    result = mkdir(path, 0700);
  // There was an error while creating the folder
  if (result != 0 && errno == EEXIST)
    printf("Folder %s already exists\n", path);
#elif _WIN32
  // Attempting to create the folder
  if ((result = CreateDirectory(path, NULL)) == 0 ||
      GetLastError() == ERROR_ALREADY_EXISTS)
    printf("Folder %s already exists\n", path);
}
#else
  fprintf(stderr, "Unsupported OS\n");
  result = -1;
#endif
  return result;
}

void sort(instance_t *instance) {
  quicksort(instance, 0, instance->number_of_jobs - 1);
}

void quicksort(instance_t *instance, int low, int high) {
  if (low >= high)
    return;
  int pi = partition(instance, low, high);

  quicksort(instance, low, pi - 1);
  quicksort(instance, pi + 1, high);
}

int partition(instance_t *instance, int low, int high) {
  int pivot = instance->release_dates[high];

  int i = low - 1;

  for (int j = low; j < high; j++) {
    if (instance->release_dates[j] < pivot) {
      i += 1;
      swap(&instance->release_dates[i], &instance->release_dates[j]);
      swap(&instance->processing_times[i], &instance->processing_times[j]);
    }
  }

  swap(&instance->release_dates[i + 1], &instance->release_dates[high]);
  swap(&instance->processing_times[i + 1], &instance->processing_times[high]);

  return i + 1;
}

void swap(int *a, int *b) {
  int t = *a;
  *a = *b;
  *b = t;
}
