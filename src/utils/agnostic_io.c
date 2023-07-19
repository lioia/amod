#include "agnostic_io.h"
#include "strings.h"
#include <errno.h>
#include <stdio.h>
#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#elif _WIN32
#define SetWindowText SetWindowTextA
#include <windows.h>
#endif

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
