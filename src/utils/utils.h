#pragma once

#include "entities.h"

char *formatted_string(const char *format, ...)
    __attribute__((format(printf, 1, 2)));

void log_error(simulation_t *sim, int result, const char *cause);

int create_folder(const char *path);

void sort(instance_t *instance, int *indexes);
