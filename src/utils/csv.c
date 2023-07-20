#include "csv.h"
#include "../generate/generate.h"
#include "entities.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int load_csv(const char *filename, vector_t *vector) {
  FILE *fp = fopen(filename, "r");
  int result = 0;
  if (fp == NULL) {
    perror(formatted_string("Could not open %s", filename));
    return -1;
  }

  // Skipping first line
  fscanf(fp, "%*[^\n]\n");

  int old_instance_number = 1;
  int number_of_jobs = 0;
  int *p_js = malloc(sizeof(*p_js) * NUMBER_OF_JOBS_UL[ARRAY_SIZE]);
  if (p_js == NULL) {
    perror("Could not allocate processing times");
    return -1;
  }

  int *r_js = malloc(sizeof(*r_js) * NUMBER_OF_JOBS_UL[2]);
  if (r_js == NULL) {
    perror("Could not allocate release dates");
    return -1;
  }

  int instance_number = 0;
  while (!feof(fp)) {
    int p_j = 0;
    int r_j = 0;
    fscanf(fp, "%d,%d,%d", &instance_number, &p_j, &r_j);
    p_js[number_of_jobs] = p_j;
    r_js[number_of_jobs] = r_j;
    number_of_jobs += 1;
    // New instance, can create the old one
    if (old_instance_number != instance_number) {
      instance_t *instance = malloc(sizeof(*instance));
      if (instance == NULL) {
        perror("Could not allocate instance");
        return -1;
      }
      instance->number_of_jobs = number_of_jobs;
      instance->processing_times = p_js;
      instance->release_dates = r_js;
      if ((result = vector_add(vector, (void **)&instance)) != 0)
        return result;

      number_of_jobs = 0;
      p_js = malloc(sizeof(*p_js) * NUMBER_OF_JOBS_UL[ARRAY_SIZE]);
      if (p_js == NULL) {
        perror("Could not allocate memory for processing times");
        return -1;
      }
      r_js = malloc(sizeof(*r_js) * NUMBER_OF_JOBS_UL[ARRAY_SIZE]);
      if (r_js == NULL) {
        perror("Could not allocate memory for release dates");
        return -1;
      }
      old_instance_number = instance_number;
    }
  }

  // Adding last loaded instance
  instance_t *instance = malloc(sizeof(*instance));
  if (instance == NULL) {
    perror("Could not allocate instance");
    return -1;
  }
  instance->number_of_jobs = number_of_jobs;
  instance->processing_times = p_js;
  instance->release_dates = r_js;

  if ((result = vector_add(vector, (void **)&instance)) != 0)
    return result;

  fclose(fp);
  return result;
}
