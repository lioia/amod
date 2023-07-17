#include "model.h"
#include "../run.h"
#include "gurobi_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *constraint_error_message(char *variable_name, size_t index);
void c_print(int c_number, int size, int *index, double *vals, char **names);
int realloc_constr(int **constr_index, double **constr_vals, size_t size);
tuple_t *create_tuple(int index, double val);

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
    return model_precedence_create(sim, instance);
  case Positional:
    return model_positional_create(sim, instance);
  case TimeIndexed:
    return model_time_indexed_create(sim, instance);
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
  solution->model = instance->model;
  solution->solver = solver;
  solution->size = instance->number_of_jobs;
  solution->values = values;

  if ((result = GRBoptimize(instance->model)) != 0) {
    size_t len =
        snprintf(NULL, 0, "Could not optimize model (instance %d, solver: %d)",
                 instance_number, solver);
    char *error = malloc(sizeof(*error) * len);
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

int model_precedence_create(simulation_t *sim, instance_t *instance) {
  int result = 0;
  int n = instance->number_of_jobs;
  int size = n                  // C_j
             + n * (n - 1) / 2; // x_(i j) i < j

  double *vars = malloc(sizeof(*vars) * size);
  if (vars == NULL) {
    fprintf(stderr, "Could not allocate memory for vars\n");
    return -1;
  }
  memset(vars, 0, sizeof(*vars) * size);
  // Setting objective function: sum_(h = 1)^n C_j
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
  for (size_t j = 0; j < n; j++) {
    size_t len = snprintf(NULL, 0, "C_%ld", j) + 1;
    names[j] = malloc(sizeof(*names[j]) * len);
    if (names[j] == NULL) {
      names[j] = "C_j";
    }
    sprintf(names[j], "C_%ld", j + 1);
    names[j][len - 1] = '\0';
  }

  for (size_t i = 0; i < n - 1; i++) {
    for (size_t j = i + 1; j < n; j++) {
      size_t len = snprintf(NULL, 0, "x_(%ld,%ld)", i, j) + 1;
      names[i + j + 2] = malloc(sizeof(*names[i + j + 2]) * len);
      if (names[i + j + 2] == NULL) {
        names[i + j + 2] = "x_(i,j)";
      } else {
        sprintf(names[i + j + 2], "x_(%ld,%ld)", i + 1, j + 1);
        names[i + j + 2][len - 1] = '\0';
      }
    }
  }

  if ((result = GRBaddvars(instance->model, size, 0, NULL, NULL, NULL, vars,
                           NULL, NULL, var_types, names)) != 0) {
    report_error(sim, "Could not add variables", result);
    return result;
  }

  int big_m = 0;
  int max_r_j = 0;
  for (size_t j = 0; j < n; j++) {
    big_m += instance->processing_times[j];
    if (instance->release_dates[j] > max_r_j)
      max_r_j = instance->release_dates[j];
  }
  big_m += max_r_j;

  size_t c_size = 1;
  int *c_index = malloc(sizeof(*c_index) * c_size);
  if (c_index == NULL) {
    fprintf(stderr, "Could not allocate memory for constr_index\n");
    return -1;
  }
  memset(c_index, 0, sizeof(*c_index) * c_size);
  double *c_vals = malloc(sizeof(*c_vals) * c_size);
  if (c_vals == NULL) {
    fprintf(stderr, "Could not allocate memory for constr_vals\n");
    return -1;
  }
  memset(c_vals, 0, sizeof(c_vals) * c_size);

  // C_j >= p_j + r_j forall j in J

  // forall j in J
  for (size_t j = 0; j < n; j++) {
    c_index[0] = j;
    c_vals[0] = 1;

    double rhs = instance->processing_times[j] + instance->release_dates[j];

    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_GREATER_EQUAL, rhs, NULL)) != 0) {
      char *error = constraint_error_message("j", j);
      report_error(sim, error, result);
      c_print(j, c_size, c_index, c_vals, names);
      return result;
    }
  }

  c_size = 3;
  if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
    return result;

  // C_i <= C_j - p_j + M(1 - x_(i j)) 1 <= i < j <= n
  for (size_t i = 0; i < n; i++) {
    for (size_t j = i + 1; j < n; j++) {
      c_index[0] = i;
      c_vals[0] = 1;
      c_index[1] = j;
      c_vals[1] = -1;
      c_index[2] = i + j + 2;
      c_vals[2] = big_m;
      double rhs = big_m - instance->processing_times[j];

      if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                                 GRB_LESS_EQUAL, rhs, NULL)) != 0) {
        char *error = constraint_error_message("i, j", j);
        report_error(sim, error, result);
        c_print(j, c_size, c_index, c_vals, names);
        return result;
      }
    }
  }

  c_size = 3;
  if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
    return result;

  // C_j <= C_i - p_i + M x_(i j) 1 <= i < j <= n
  for (size_t i = 0; i < n; i++) {
    for (size_t j = i + 1; j < n; j++) {
      c_index[0] = j;
      c_vals[0] = 1;
      c_index[1] = i;
      c_vals[1] = -1;
      c_index[2] = i + j + 2;
      c_vals[2] = -big_m;
      double rhs = -instance->processing_times[i];

      if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                                 GRB_LESS_EQUAL, rhs, NULL)) != 0) {
        char *error = constraint_error_message("i, j", j);
        report_error(sim, error, result);
        c_print(j, c_size, c_index, c_vals, names);
        return result;
      }
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
      size_t index = n + j * n + h;
      size_t len = snprintf(NULL, 0, "x_(%ld,%ld)", j + 1, h + 1) + 1;
      names[index] = malloc(sizeof(*names[index]) * len);
      if (names[index] == NULL) {
        names[index] = "x_(j,h)";
      } else {
        sprintf(names[index], "x_(%ld,%ld)", j + 1, h + 1);
        names[index][len - 1] = '\0';
      }
    }
  }

  if ((result = GRBaddvars(instance->model, size, 0, NULL, NULL, NULL, vars,
                           NULL, NULL, var_types, names)) != 0) {
    report_error(sim, "Could not add variables", result);
    return result;
  }

  size_t c_size = n;
  int *c_index = malloc(sizeof(*c_index) * c_size);
  if (c_index == NULL) {
    fprintf(stderr, "Could not allocate memory for constr_index\n");
    return -1;
  }
  memset(c_index, 0, sizeof(*c_index) * c_size);
  double *c_vals = malloc(sizeof(*c_vals) * c_size);
  if (c_vals == NULL) {
    fprintf(stderr, "Could not allocate memory for constr_vals\n");
    return -1;
  }
  memset(c_vals, 0, sizeof(c_vals) * c_size);

  // sum_(h=1)^n x_(j h) = 1 forall j in J

  // forall j in J
  for (size_t j = 0; j < n; j++) {
    for (size_t h = 0; h < n; h++) {
      c_index[h] = n + j * n + h;
      c_vals[h] = 1;
    }
    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_EQUAL, 1, NULL)) != 0) {
      char *error = constraint_error_message("j", j);
      report_error(sim, error, result);
      c_print(j, c_size, c_index, c_vals, names);
      return result;
    }
  }

  c_size = n;
  if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
    return result;

  // sum_(j in J) x_(j h) forall h=1,..,n

  // forall h=1,...n
  for (size_t h = 0; h < n; h++) {
    for (size_t j = 0; j < n; j++) {
      c_index[j] = n + j * n + h;
      c_vals[j] = 1;
    }
    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_EQUAL, 1, NULL)) != 0) {
      char *error = constraint_error_message("h", h);
      report_error(sim, error, result);
      c_print(h, c_size, c_index, c_vals, names);
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
    c_print(1, c_size, c_index, c_vals, names);
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
      report_error(sim, error, result);
      c_print(h, c_size, c_index, c_vals, names);
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
      c_print(h, c_size, c_index, c_vals, names);
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
      c_print(h, c_size, c_index, c_vals, names);
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

int model_time_indexed_create(simulation_t *sim, instance_t *instance) {
  int result = 0;
  int n = instance->number_of_jobs;

  int big_t = 0;
  int max_r_j = 0;
  for (size_t j = 0; j < n; j++) {
    big_t += instance->processing_times[j];
    if (instance->release_dates[j] > max_r_j)
      max_r_j = instance->release_dates[j];
  }
  big_t += max_r_j;

  vector_t *tuples = vector_init();
  if (tuples == NULL)
    return -1;
  for (size_t j = 0; j < n; j++) {
    int number_of_vars = big_t - instance->processing_times[j] + 1;
    for (size_t t = 0; t < number_of_vars; t++) {
      tuple_t *tuple =
          create_tuple(0, t + 1 + instance->processing_times[j] - 1);
      if (tuple == NULL)
        return -1;
      if ((result = vector_add(tuples, (void **)&tuple)) != 0)
        return result;
    }
  }

  double *vars = malloc(sizeof(*vars) * tuples->length);
  if (vars == NULL) {
    fprintf(stderr, "Could not allocate memory for vars\n");
    return -1;
  }
  for (size_t i = 0; i < tuples->length; i++) {
    tuple_t *tuple = tuples->values[i];
    vars[i] = tuple->val;
  }

  char *var_types = malloc(sizeof(*var_types) * tuples->length);
  if (var_types == NULL) {
    fprintf(stderr, "Could not allocate memory for var_types\n");
    return -1;
  }
  memset(var_types, GRB_BINARY, sizeof(*var_types) * tuples->length);

  char **names = malloc(sizeof(*names) * tuples->length);
  if (names == NULL) {
    fprintf(stderr, "Could not allocate memory for names\n");
    return -1;
  }
  size_t index = 0;
  for (size_t j = 0; j < n; j++) {
    for (size_t t = 0; t < big_t - instance->processing_times[j] + 1; t++) {
      size_t len = snprintf(NULL, 0, "x_(%ld,%ld)", j + 1, t + 1) + 1;
      names[index] = malloc(sizeof(*names[index]) * len);
      if (names[index] == NULL) {
        names[index] = "x_(j,t)";
      } else {
        sprintf(names[index], "x_(%ld,%ld)", j + 1, t + 1);
        names[index][len - 1] = '\0';
      }
      index += 1;
    }
  }

  if ((result = GRBaddvars(instance->model, tuples->length, 0, NULL, NULL, NULL,
                           vars, NULL, NULL, var_types, names)) != 0) {
    report_error(sim, "Could not add variables", result);
    return result;
  }

  size_t c_size = 1;
  int *c_index = malloc(sizeof(*c_index) * c_size);
  if (c_index == NULL) {
    fprintf(stderr, "Could not allocate memory for constr_index\n");
    return -1;
  }
  memset(c_index, 0, sizeof(*c_index) * c_size);
  double *c_vals = malloc(sizeof(*c_vals) * c_size);
  if (c_vals == NULL) {
    fprintf(stderr, "Could not allocate memory for constr_vals\n");
    return -1;
  }
  memset(c_vals, 0, sizeof(c_vals) * c_size);

  // sum_(t = 1)^(T - p_j + 1) x_(j t) = 1 forall j in J
  int offset_j = 0;
  for (size_t j = 0; j < n; j++) {
    c_size = big_t - instance->processing_times[j] + 1;

    if ((result = vector_reset(tuples)) != 0)
      return result;

    for (size_t t = 0; t < c_size; t++) {
      tuple_t *tuple = create_tuple(offset_j + t, 1);
      if (tuple == NULL)
        return -1;
      if ((result = vector_add(tuples, (void **)&tuple)) != 0)
        return result;
    }

    if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
      return result;

    for (size_t i = 0; i < tuples->length; i++) {
      tuple_t *tuple = tuples->values[i];
      c_index[i] = tuple->index;
      c_vals[i] = tuple->val;
    }

    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_EQUAL, 1, NULL)) != 0) {
      char *error = constraint_error_message("j", j);
      report_error(sim, error, result);
      c_print(j, c_size, c_index, c_vals, names);
      return result;
    }
    offset_j += big_t - instance->processing_times[j] + 1;
  }

  // sum_(j in J) sum_(t = max{0,tau-p_j+1})^T x_(j t) <= 1 forall tau=1,...T
  for (size_t tau = 0; tau < big_t; tau++) {
    if ((result = vector_reset(tuples)) != 0)
      return -1;
    offset_j = 0;
    for (size_t j = 0; j < n; j++) {
      int max = tau - instance->processing_times[j] + 1;
      if (max < 0)
        max = 0;
      if (big_t - tau < instance->processing_times[j]) {
        offset_j += big_t - instance->processing_times[j] + 1;
        continue;
      }
      for (size_t t = max; t <= tau; t++) {
        tuple_t *tuple = create_tuple(offset_j + t, 1);
        if (tuple == NULL)
          return -1;
        if ((result = vector_add(tuples, (void **)&tuple)) != 0)
          return result;
      }
      offset_j += big_t - instance->processing_times[j] + 1;
    }
    if (tuples->length == 0)
      continue;

    if ((result = realloc_constr(&c_index, &c_vals, tuples->length)) != 0)
      return result;

    for (size_t i = 0; i < tuples->length; i++) {
      tuple_t *tuple = tuples->values[i];
      c_index[i] = tuple->index;
      c_vals[i] = tuple->val;
    }

    if ((result = GRBaddconstr(instance->model, tuples->length, c_index, c_vals,
                               GRB_LESS_EQUAL, 1, NULL)) != 0) {
      char *error = constraint_error_message("tau", tau);
      report_error(sim, error, result);
      c_print(tau, tuples->length, c_index, c_vals, names);
      return result;
    }
  }

  // Release times
  offset_j = 0;
  for (size_t j = 0; j < n; j++) {
    c_size = instance->release_dates[j];

    if (c_size == 0) {
      offset_j += big_t - instance->processing_times[j] + 1;
      continue;
    }
    if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
      return result;

    for (size_t t = 0; t < c_size; t++) {
      c_index[t] = offset_j + t;
      c_vals[t] = 1;
    }
    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_EQUAL, 0, NULL)) != 0) {
      char *error = constraint_error_message("release dates j", j);
      report_error(sim, error, result);
      c_print(j, c_size, c_index, c_vals, names);
      return result;
    }
    offset_j += big_t - instance->processing_times[j] + 1;
  }

  free(tuples);
  tuples = NULL;
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

void c_print(int contraint_index, int size, int *index, double *vals,
             char **names) {
  printf("Constraint: %d\n", contraint_index);
  for (size_t i = 0; i < size; i++) {
    char *variable_name = names[index[i]];
    char *multiplier = "";
    if (vals[i] > 0) {
      size_t len = snprintf(NULL, 0, "+%.2f*", vals[i]) + 1;
      multiplier = malloc(sizeof(*multiplier) * len);
      if (multiplier == NULL)
        multiplier = "";
      else
        sprintf(multiplier, "+%.2f*", vals[i]);

    } else {
      size_t len = snprintf(NULL, 0, "%.2f*", vals[i]) + 1;
      multiplier = malloc(sizeof(*multiplier) * len);
      if (multiplier == NULL)
        multiplier = "";
      else
        sprintf(multiplier, "%.2f*", vals[i]);
    }
    if (vals[i] == 1)
      multiplier = "+";
    printf("%s %s ", multiplier, variable_name);

    if (i == size - 1)
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

tuple_t *create_tuple(int index, double val) {
  tuple_t *tuple = malloc(sizeof(*tuple));
  if (tuple == NULL) {
    fprintf(stderr, "Could not allocate memory for tuple\n");
    return NULL;
  }
  tuple->index = index;
  tuple->val = val;
  return tuple;
}
