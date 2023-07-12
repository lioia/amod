#pragma once

#include "gurobi_c.h"
#include "vector.h"

typedef enum {
  Precedence,
  Positional,
  TimeIndexed,
  Heuristics
} resolution_method_t;

typedef struct {
  int number_of_jobs;
  int *processing_times;
  int *release_dates;
} instance_t;

typedef struct {
  GRBenv *gurobi_env;
  GRBmodel *gurobi_model;
  vector_t *instances;
} simulation_t;
