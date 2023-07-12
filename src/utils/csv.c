#include "csv.h"
#include "../generate/generate.h"
#include "entities.h"
#include "vector.h"
#include <stdio.h>
#include <stdlib.h>

// vector is already initialized
int load_csv(const char *filename, vector_t *vector) {
  FILE *fp = fopen(filename, "r");
  int result = 0;
  if (fp == NULL) {
    fprintf(stderr, "Could not open %s\n", filename);
    return result;
  }

  // Skipping first line
  fscanf(fp, "%*[^\n]\n");

  int old_instance_number = 1;
  int number_of_jobs = 0;
  int *processing_times =
      malloc(sizeof(*processing_times) * NUMBER_OF_JOBS_UL[2]);
  if (processing_times == NULL) {
    fprintf(stderr, "Could not allocate processing times\n");
    return -1;
  }

  int *release_dates = malloc(sizeof(*release_dates) * NUMBER_OF_JOBS_UL[2]);
  if (release_dates == NULL) {
    fprintf(stderr, "Could not allocate release dates\n");
    return -1;
  }

  while (!feof(fp)) {
    int instance_number = 0;
    int processing_time = 0;
    int release_date = 0;

    fscanf(fp, "%d,%d,%d\n", &instance_number, &processing_time, &release_date);
    processing_times[number_of_jobs] = processing_time;
    release_dates[number_of_jobs] = release_date;
    number_of_jobs += 1;
    // New instance, can create the old one
    if (old_instance_number != instance_number) {
      instance_t *instance = malloc(sizeof(*instance));
      if (instance == NULL) {
        fprintf(stderr, "Could not allocate instance\n");
        return -1;
      }
      instance->number_of_jobs = number_of_jobs;
      instance->processing_times = processing_times;
      instance->release_dates = release_dates;
      if ((result = vector_add(vector, instance)) != 0)
        return result;
      number_of_jobs = 0;
      processing_times =
          malloc(sizeof(*processing_times) * NUMBER_OF_JOBS_UL[2]);
      release_dates = malloc(sizeof(*release_dates) * NUMBER_OF_JOBS_UL[2]);
      old_instance_number = instance_number;
    }
  }

  // Adding last loaded instance
  instance_t *instance = malloc(sizeof(*instance));
  if (instance == NULL) {
    fprintf(stderr, "Could not allocate instance\n");
    return -1;
  }
  instance->number_of_jobs = number_of_jobs;
  instance->processing_times = processing_times;
  instance->release_dates = release_dates;

  if ((result = vector_add(vector, instance)) != 0)
    return result;

  fclose(fp);
  return result;
}
