#include "model.h"
#include "../run.h"
#include "gurobi_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *constraint_error_message(char *variable_name, size_t index);
void print_constraint(int number, int size, int *index, double *vals);
int realloc_constr(int **constr_index, double **constr_vals, size_t size);

int model_init(simulation_t *sim, int instance_number, solver_t solver) {
  instance_t *instance = sim->instances->values[instance_number];

  int result = 0;
  size_t len = snprintf(NULL, 0, "%d-%d", solver, instance_number) + 1;
  char *name = malloc(sizeof(*name) * len);
  if (name == NULL)
    name = "unknown-unknown";
  else {
    sprintf(name, "%d-%d", solver, instance_number);
    name[len - 1] = '\0';
  }

  if ((result = GRBnewmodel(sim->env, &instance->model, name, 0, NULL, NULL,
                            NULL, NULL, NULL)) != 0) {
    report_error(sim, "Could not create new Gurobi model", result);
    return result;
  }

  switch (solver) {
  case Precedence:
    break;
  case Positional:
    return model_positional_create(sim, instance);
    break;
  case TimeIndexed:
    break;
    // TODO: Heuristics case
  }
  return 0;
}

solution_t *model_optimize(simulation_t *sim, int instance_number,
                           solver_t solver) {
  int result;
  instance_t *instance = sim->instances->values[instance_number];

  double *values = malloc(sizeof(*values) * instance->number_of_jobs);
  if (values == NULL) {
    fprintf(stderr, "Could not allocate memory for solution values\n");
    return NULL;
  }

  solution_t *solution = malloc(sizeof(*solution));
  if (solution == NULL) {
    fprintf(stderr, "Could not allocate memory for solution\n");
  }
  memset(solution, 0, sizeof(*solution));
  solution->values = values;

  if ((result = GRBoptimize(instance->model)) != 0) {
    size_t str_len =
        snprintf(NULL, 0, "Could not optimize model (instance %d, solver: %d)",
                 instance_number, solver);
    char *error = malloc(sizeof(*error) * str_len);
    if (error == NULL)
      error = "Could not optimize model";
    report_error(sim, error, result);
    return NULL;
  }

  if ((result = GRBgetintattr(instance->model, GRB_INT_ATTR_STATUS,
                              &solution->status)) != 0) {
    report_error(sim, "Could not get status", result);
    return NULL;
  }

  if ((result = GRBgetdblattr(instance->model, GRB_DBL_ATTR_OBJVAL,
                              &solution->objective_value)) != 0) {
    report_error(sim, "Could not get objective value", result);
  }

  if ((result = GRBgetdblattrarray(instance->model, GRB_DBL_ATTR_X, 0,
                                   instance->number_of_jobs,
                                   solution->values)) != 0) {
    report_error(sim, "Could not get solution values", result);
  }

  return solution;
}

int model_positional_create(simulation_t *sim, instance_t *instance) {
  int result = 0;

  int n = instance->number_of_jobs;
  int size = n +    // C_[h]
             n * n; // x_(j h)

  double *vars = malloc(sizeof(*vars) * size);
  if (vars == NULL) {
    fprintf(stderr, "Could not allocate memory for vars\n");
    return -1;
  }
  memset(vars, 0, sizeof(*vars) * size);
  // Setting objective function: sum_(h = 1)^n C_[h]
  for (size_t i = 0; i < n; i++) {
    vars[i] = 1;
  }

  char *var_types = malloc(sizeof(*var_types) * size);
  if (var_types == NULL) {
    fprintf(stderr, "Could not allocate memory for var_types\n");
    return -1;
  }
  memset(var_types, GRB_BINARY, sizeof(*var_types) * size);
  for (size_t i = 0; i < n; i++) {
    var_types[i] = GRB_INTEGER;
  }

  char **names = malloc(sizeof(*names) * size);
  if (names == NULL) {
    fprintf(stderr, "Could not allocate memory for names\n");
    return -1;
  }
  for (size_t h = 0; h < n; h++) {
    size_t len = snprintf(NULL, 0, "C_%ld", h) + 1;
    names[h] = malloc(sizeof(*names[h]) * len);
    if (names[h] == NULL) {
      names[h] = "C_h";
    }
    sprintf(names[h], "C_%ld", h + 1);
    names[h][len - 1] = '\0';
  }

  for (size_t j = 0; j < n; j++) {
    for (size_t h = 0; h < n; h++) {
      size_t len = snprintf(NULL, 0, "x_(%ld,%ld)", j, h) + 1;
      names[n + j * n + h] = malloc(sizeof(*names[n + j * n + h]) * len);
      if (names[n + j * n + h] == NULL) {
        names[n + j * n + h] = "x_(j,h)";
      } else {
        sprintf(names[n + j * n + h], "x_(%ld,%ld)", j + 1, h + 1);
        names[n + j * n + h][len - 1] = '\0';
      }
    }
  }

  if ((result = GRBaddvars(instance->model, size, 0, NULL, NULL, NULL, vars,
                           NULL, NULL, var_types, names)) != 0) {
    report_error(sim, "Could not add variables", result);
    return result;
  }

  // sum_(h=1)^n x_(j h) = 1 forall j in J

  // forall j in J
  int *c_index = malloc(sizeof(*c_index) * n);
  if (c_index == NULL) {
    fprintf(stderr, "Could not allocate memory for constr_index\n");
    return -1;
  }
  memset(c_index, 0, sizeof(*c_index) * n);
  double *c_vals = malloc(sizeof(*c_vals) * n);
  if (c_vals == NULL) {
    fprintf(stderr, "Could not allocate memory for constr_vals\n");
    return -1;
  }
  memset(c_vals, 0, sizeof(c_vals) * n);

  for (size_t j = 0; j < n; j++) {
    for (size_t h = 0; h < n; h++) {
      c_index[h] = n + j * n + h;
      c_vals[h] = 1;
    }
    if ((result = GRBaddconstr(instance->model, n, c_index, c_vals, GRB_EQUAL,
                               1, NULL)) != 0) {
      char *error = constraint_error_message("j", j);
      report_error(sim, error, result);
      return result;
    }
  }

  int c_size = n;
  if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
    return result;

  // sum_(j in J) x_(j h) forall h=1,..,n

  // forall h=1,...n
  for (size_t h = 0; h < n; h++) {
    for (size_t j = 0; j < n; j++) {
      c_index[j] = n + j * n + h;
      c_vals[j] = 1;
    }
    print_constraint(h, c_size, c_index, c_vals);
    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_EQUAL, 1, NULL)) != 0) {
      char *error = constraint_error_message("h", h);
      report_error(sim, error, result);
      return result;
    }
  }

  c_size = n + 1;
  if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
    return result;

  // C_1 >= sum_(j in J) (p_j x_(j 1))
  c_index[0] = 0;
  c_vals[0] = 1;
  for (size_t j = 0; j < n; j++) {
    c_index[1 + j] = n + j * n;
    c_vals[1 + j] = -instance->processing_times[j];
  }

  if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                             GRB_GREATER_EQUAL, 0, NULL)) != 0) {
    char *error = constraint_error_message("1", 1);
    report_error(sim, error, result);
    return result;
  }

  // C_[h] >= C_[h - 1] + sum_(j in J) (p_j x(j h)) forall h=2,...,n
  for (size_t h = 1; h < n; h++) {
    c_size = n + 2;
    if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
      return result;
    // C_[h]
    c_index[0] = h;
    c_vals[0] = 1;
    // C_[h-1]
    c_index[1] = h - 1;
    c_vals[1] = -1;
    for (size_t j = 0; j < n; j++) {
      c_index[2 + j] = n + j * n + h;
      c_vals[2 + j] = -instance->processing_times[j];
    }
    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_GREATER_EQUAL, 0, NULL)) != 0) {
      char *error = constraint_error_message("h", h);
      printf("Error: %s\n", error);
      report_error(sim, error, result);
      return result;
    }
  }

  c_size = n + 1;
  if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
    return result;

  // C_[h] >= sum_(j in J) (p_j + r_j) * x_(j h)
  for (size_t h = 0; h < n; h++) {
    c_index[0] = h;
    c_vals[0] = 1;
    for (size_t j = 0; j < n; j++) {
      int c_j = instance->processing_times[j] + instance->release_dates[j];
      c_index[1 + j] = n + j * n + h;
      c_vals[1 + j] = -c_j;
    }
    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_GREATER_EQUAL, 0, NULL)) != 0) {
      char *error = constraint_error_message("h", h);
      report_error(sim, error, result);
      return result;
    }
  }

  c_size = 1;
  if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
    return -1;

  // C_[h] >= 0
  for (size_t h = 0; h < n; h++) {
    c_index[0] = h;
    c_vals[0] = 1;
    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_GREATER_EQUAL, 0, NULL)) != 0) {
      char *error = constraint_error_message("h", h);
      report_error(sim, error, result);
      return result;
    }
  }

  free(vars);
  vars = NULL;
  free(var_types);
  var_types = NULL;
  free(c_index);
  c_index = NULL;
  free(c_vals);
  c_vals = NULL;

  return result;
}

char *constraint_error_message(char *variable_name, size_t index) {
  size_t len = snprintf(NULL, 0, "Could not add constraint for %s = %ld",
                        variable_name, index);
  char *error = malloc(sizeof(*error) * (len + 1));
  if (error == NULL) {
    return "Could not add constraint";
  }
  sprintf(error, "Could not add constraint for %s = %ld", variable_name, index);
  error[len] = '\0';
  return error;
}

void print_constraint(int number, int size, int *index, double *vals) {
  printf("Constraint: %d\n", number);
  for (size_t i = 0; i < size; i++) {
    printf("%ld: (%d %f)", i, index[i], vals[i]);
    if (i != size - 1)
      printf(", ");
    else
      printf("\n");
  }
}

int realloc_constr(int **constr_index, double **constr_vals, size_t size) {
  *constr_index = realloc(*constr_index, sizeof(**constr_index) * size);
  if (*constr_index == NULL) {
    fprintf(stderr, "Could not realloc constr_index\n");
    return -1;
  }
  memset(*constr_index, 0, sizeof(**constr_index) * size);

  *constr_vals = realloc(*constr_vals, sizeof(**constr_vals) * size);
  if (*constr_vals == NULL) {
    fprintf(stderr, "Could not realloc constr_vals\n");
    return -1;
  }
  memset(*constr_vals, 0, sizeof(**constr_vals) * size);

  return 0;
}
