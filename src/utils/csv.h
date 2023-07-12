#pragma once

#include "entities.h"
#include "vector.h"

int load_csv(const char *filename, vector_t *vector);
int instance_init(instance_t *instance, int number_of_jobs,
                  int *processing_times, int *release_dates);
