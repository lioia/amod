#pragma once

#include "gurobi_c.h"
#define VECTOR_DEFAULT_SIZE 32

typedef enum { Precedence, Positional, TimeIndexed /*, Heuristics*/ } solver_t;

typedef struct {
  int number_of_jobs;
  int *processing_times;
  int *release_dates;
  GRBmodel *model;
} instance_t;

typedef struct {
  int allocated_length;
  int length; // Number of elements in the vector
  void **values;
} vector_t;

typedef struct {
  GRBenv *env;
  vector_t *instances;
} simulation_t;

typedef struct {
  size_t size;            // Number of variables
  solver_t solver;        // Type of model used
  int status;             // Status code from Gurobi
  double runtime;         // Execution time
  double objective_value; // z*
  double *values;         // x*
} solution_t;

// Create new vector
vector_t *vector_init();
// Add `value` to `vector` (resizing if necessary)
int vector_add(vector_t *vector, void **value);
// Clean every value saved in vector and vector itself
int vector_free(vector_t *vector);
// Resize vector values to size `length`
int vector_refit(vector_t *vector);
// Reinit the vector
int vector_reset(vector_t *vector);
