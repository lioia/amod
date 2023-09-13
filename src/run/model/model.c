#include "model.h"
#include "../../utils/utils.h"
#include "../run.h"
#include "gurobi_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void c_print(int size, int *index, double *vals, char **names);
int realloc_constr(int **constr_index, double **constr_vals, size_t size);
tuple_t *create_tuple(int index, double val);

int model_init(simulation_t *sim, int instance_number, solver_t solver,
               int *heuristic_value) {
  instance_t *instance = sim->instances->values[instance_number];
  int result = 0;

  char *name = formatted_string("%d,%d", solver, instance_number);
  if (name == NULL)
    name = "unknown,unknown";

  if ((result = GRBnewmodel(sim->env, &instance->model, name, 0, NULL, NULL,
                            NULL, NULL, NULL)) != 0) {
    log_error(sim, result, "GRBnewmodel");
    return result;
  }

  if ((result = GRBsetdblparam(GRBgetenv(instance->model),
                               GRB_DBL_PAR_TIMELIMIT, TIME_LIMIT)) != 0) {
    log_error(sim, result, "GRBsetdblparam(\"GRB_DBL_PAR_TIMELIMIT\")");
    return result;
  }

  switch (solver) {
  case Precedence:
    result = model_precedence_create(sim, instance);
    break;
  case Positional:
    result = model_positional_create(sim, instance);
    break;
  case TimeIndexed:
    result = model_time_indexed_create(sim, instance);
    break;
  case Heuristics:
    result = model_heuristics_create(sim, instance, heuristic_value);
    break;
  }
  free(name);
  name = NULL;
  return result;
}

solution_t *model_optimize(simulation_t *sim, int i, solver_t solver) {
  int result;
  instance_t *instance = sim->instances->values[i];

  double *values = malloc(sizeof(*values) * instance->number_of_jobs);
  if (values == NULL) {
    perror("Could not allocate memory for solution values");
    return NULL;
  }

  solution_t *solution = malloc(sizeof(*solution));
  if (solution == NULL) {
    perror("Could not allocate memory for solution");
    return NULL;
  }
  memset(solution, 0, sizeof(*solution));
  solution->solver = solver;
  solution->size = instance->number_of_jobs;
  solution->values = values;
  solution->heuristic_value = -1;

  if ((result = GRBoptimize(instance->model)) != 0) {
    log_error(sim, result, "GRBoptimize");
    return NULL;
  }

  if ((result = GRBgetintattr(instance->model, GRB_INT_ATTR_STATUS,
                              &solution->status)) != 0)
    log_error(sim, result, "GRBgetintattr(\"GRB_INT_ATTR_STATUS\")");

  if ((result = GRBgetdblattr(instance->model, GRB_DBL_ATTR_RUNTIME,
                              &solution->runtime)) != 0)
    log_error(sim, result, "GRBgetdblattr(\"GRB_DBL_ATTR_RUNTIME\")");

  if ((result = GRBgetdblattr(instance->model, GRB_DBL_ATTR_OBJVAL,
                              &solution->objective_value)) != 0)
    log_error(sim, result, "GRBgetdblattr(\"GRB_DBL_ATTR_OBJVAL\")");

  if ((result =
           GRBgetdblattrarray(instance->model, GRB_DBL_ATTR_X, 0,
                              instance->number_of_jobs, solution->values)) != 0)
    log_error(sim, result, "GRBgetdblattrarray(\"GRB_DBL_ATTR_X\")");

  return solution;
}

int model_precedence_create(simulation_t *sim, instance_t *instance) {
  int result = 0;
  int n = instance->number_of_jobs;
  int size = n                  // C_j
             + n * (n - 1) / 2; // x_(i j) i < j

  double *vars = malloc(sizeof(*vars) * size);
  if (vars == NULL) {
    perror("Could not allocate memory for vars");
    return -1;
  }
  memset(vars, 0, sizeof(*vars) * size);
  // Setting objective function: sum_(h = 1)^n C_j
  for (size_t i = 0; i < n; i++) {
    vars[i] = 1.0;
  }

  char *var_types = malloc(sizeof(*var_types) * size);
  if (var_types == NULL) {
    perror("Could not allocate memory for var_types");
    return -1;
  }
  memset(var_types, GRB_BINARY, sizeof(*var_types) * size);
  memset(var_types, GRB_INTEGER, sizeof(*var_types) * n);

  char **names = malloc(sizeof(*names) * size);
  if (names == NULL) {
    perror("Could not allocate memory for names");
    return -1;
  }
  for (size_t j = 0; j < n; j++) {
    char *name = formatted_string("C_%ld", j + 1);
    if (name == NULL)
      name = "C_j";
    names[j] = name;
  }

  size_t index = n;
  for (size_t i = 0; i < n; i++) {
    for (size_t j = i + 1; j < n; j++) {
      char *name = formatted_string("x_(%ld,%ld)", i + 1, j + 1);
      if (name == NULL)
        name = "x_(i,j)";
      names[index] = name;
      index += 1;
    }
  }

  if ((result = GRBaddvars(instance->model, size, 0, NULL, NULL, NULL, vars,
                           NULL, NULL, var_types, names)) != 0) {
    log_error(sim, result, "GRBaddvars");
    return result;
  }

  // M > sum_(j in J) p_j + max{r_j}
  int big_m = 1;
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
    perror("Could not allocate memory for constr_index");
    return -1;
  }
  memset(c_index, 0, sizeof(*c_index) * c_size);
  double *c_vals = malloc(sizeof(*c_vals) * c_size);
  if (c_vals == NULL) {
    perror("Could not allocate memory for constr_vals");
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
      perror(formatted_string("Constraint C_j >= p_j + r_j: j = %ld", j));
      log_error(sim, result, "GRBaddconstr");
      c_print(c_size, c_index, c_vals, names);
      return result;
    }
  }

  c_size = 3;
  if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
    return result;

  // C_i <= C_j - p_j + M(1 - x_(i j)) 1 <= i < j <= n
  index = n;
  for (size_t i = 0; i < n; i++) {
    for (size_t j = i + 1; j < n; j++) {
      c_index[0] = i;
      c_vals[0] = 1;
      c_index[1] = j;
      c_vals[1] = -1;
      c_index[2] = index++;
      c_vals[2] = big_m;
      double rhs = big_m - instance->processing_times[j];

      if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                                 GRB_LESS_EQUAL, rhs, NULL)) != 0) {
        perror(formatted_string("big M constraint 1: i = %ld, j = %ld", i, j));
        log_error(sim, result, "GRBaddconstr");
        c_print(c_size, c_index, c_vals, names);
        return result;
      }
    }
  }

  c_size = 3;
  if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
    return result;

  // C_j <= C_i - p_i + M x_(i j) 1 <= i < j <= n
  index = n;
  for (size_t i = 0; i < n; i++) {
    for (size_t j = i + 1; j < n; j++) {
      c_index[0] = j;
      c_vals[0] = 1;
      c_index[1] = i;
      c_vals[1] = -1;
      c_index[2] = index++;
      c_vals[2] = -big_m;
      double rhs = -instance->processing_times[i];

      if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                                 GRB_LESS_EQUAL, rhs, NULL)) != 0) {
        perror(formatted_string("big M constraint 2: i = %ld, j = %ld", i, j));
        log_error(sim, result, "GRBaddconstr");
        c_print(c_size, c_index, c_vals, names);
        return result;
      }
    }
  }

  for (size_t i = 0; i < size; i++) {
    free(names[i]);
  }
  free(names);
  names = NULL;
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
    perror("Could not allocate memory for vars");
    return -1;
  }
  memset(vars, 0, sizeof(*vars) * size);
  // Setting objective function: sum_(h = 1)^n C_[h]
  for (size_t i = 0; i < n; i++) {
    vars[i] = 1;
  }

  char *var_types = malloc(sizeof(*var_types) * size);
  if (var_types == NULL) {
    perror("Could not allocate memory for var_types");
    return -1;
  }
  memset(var_types, GRB_BINARY, sizeof(*var_types) * size);
  for (size_t i = 0; i < n; i++) {
    var_types[i] = GRB_INTEGER;
  }

  char **names = malloc(sizeof(*names) * size);
  if (names == NULL) {
    perror("Could not allocate memory for names");
    return -1;
  }
  for (size_t h = 0; h < n; h++) {
    char *name = formatted_string("C_%ld", h + 1);
    if (name == NULL)
      name = "C_[h]";
    names[h] = name;
  }

  for (size_t j = 0; j < n; j++) {
    for (size_t h = 0; h < n; h++) {
      size_t index = n + j * n + h;
      char *name = formatted_string("x_(%ld,%ld)", j + 1, h + 1);
      if (name == NULL)
        name = "x_(j,h)";
      names[n + j * n + h] = name;
    }
  }
  if ((result = GRBaddvars(instance->model, size, 0, NULL, NULL, NULL, vars,
                           NULL, NULL, var_types, names)) != 0) {
    log_error(sim, result, "GRBaddvars");
    return result;
  }

  size_t c_size = n;
  int *c_index = malloc(sizeof(*c_index) * c_size);
  if (c_index == NULL) {
    perror("Could not allocate memory for constr_index");
    return -1;
  }
  memset(c_index, 0, sizeof(*c_index) * c_size);
  double *c_vals = malloc(sizeof(*c_vals) * c_size);
  if (c_vals == NULL) {
    perror("Could not allocate memory for constr_vals");
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
      perror(formatted_string("Constraint j = %ld", j));
      log_error(sim, result, "GRBaddconstr");
      c_print(c_size, c_index, c_vals, names);
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
      perror(formatted_string("Constraint h = %ld", h));
      log_error(sim, result, "GRBaddconstr");
      c_print(c_size, c_index, c_vals, names);
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
    perror("Constraint C_1");
    log_error(sim, result, "GRBaddconstr");
    c_print(c_size, c_index, c_vals, names);
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
      perror(formatted_string("Constraint h = %ld", h));
      log_error(sim, result, "GRBaddconstr");
      c_print(c_size, c_index, c_vals, names);
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
      perror(formatted_string("Constraint h = %ld", h));
      log_error(sim, result, "GRBaddconstr");
      c_print(c_size, c_index, c_vals, names);
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
      perror(formatted_string("Constraint h = %ld", h));
      log_error(sim, result, "GRBaddconstr");
      c_print(c_size, c_index, c_vals, names);
      return result;
    }
  }

  for (size_t i = 0; i < size; i++) {
    free(names[i]);
  }
  free(names);
  names = NULL;
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

  int big_t = 1;
  int max_r_j = 0;
  for (size_t j = 0; j < n; j++) {
    big_t += instance->processing_times[j];
    if (instance->release_dates[j] > max_r_j)
      max_r_j = instance->release_dates[j];
  }
  big_t += max_r_j;

  int size = 0;
  for (size_t j = 0; j < n; j++) {
    size += big_t - instance->processing_times[j] + 1;
  }

  size_t index = 0;
  double *vars = malloc(sizeof(*vars) * size);
  if (vars == NULL) {
    perror("Could not allocate memory for vars");
    return -1;
  }
  for (size_t j = 0; j < n; j++) {
    for (size_t t = 0; t < big_t - instance->processing_times[j] + 1; t++) {
      vars[index] = t + 1 + instance->processing_times[j] - 1;
      index += 1;
    }
  }

  char *var_types = malloc(sizeof(*var_types) * size);
  if (var_types == NULL) {
    perror("Could not allocate memory for var_types");
    return -1;
  }
  memset(var_types, GRB_BINARY, sizeof(*var_types) * size);

  char **names = malloc(sizeof(*names) * size);
  if (names == NULL) {
    perror("Could not allocate memory for names");
    return -1;
  }

  index = 0;
  for (size_t j = 0; j < n; j++) {
    for (size_t t = 0; t < big_t - instance->processing_times[j] + 1; t++) {
      char *name = formatted_string("x_(%ld,%ld)", j + 1, t + 1);
      if (name == NULL)
        name = "x_(j,t)";
      names[index++] = name;
    }
  }

  if ((result = GRBaddvars(instance->model, size, 0, NULL, NULL, NULL, vars,
                           NULL, NULL, var_types, names)) != 0) {
    log_error(sim, result, "GRBaddvars");
    return result;
  }

  size_t c_size = 1;
  int *c_index = malloc(sizeof(*c_index) * c_size);
  if (c_index == NULL) {
    perror("Could not allocate memory for constr_index");
    return -1;
  }
  memset(c_index, 0, sizeof(*c_index) * c_size);
  double *c_vals = malloc(sizeof(*c_vals) * c_size);
  if (c_vals == NULL) {
    perror("Could not allocate memory for constr_vals");
    return -1;
  }
  memset(c_vals, 0, sizeof(c_vals) * c_size);

  // sum_(t = 1)^(T - p_j + 1) x_(j t) = 1 forall j in J
  int offset_j = 0;
  for (size_t j = 0; j < n; j++) {
    index = 0;
    c_size = big_t - instance->processing_times[j] + 1;

    if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
      return result;

    for (size_t t = 0; t < c_size; t++) {
      c_index[index] = offset_j + t;
      c_vals[index] = 1;
      index += 1;
    }

    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_EQUAL, 1, NULL)) != 0) {
      perror(formatted_string("Constraint j = %ld", j));
      log_error(sim, result, "GRBaddconstr");
      c_print(c_size, c_index, c_vals, names);
      return result;
    }
    offset_j += big_t - instance->processing_times[j] + 1;
  }

  // sum_(j in J) sum_(t = max{0,tau-p_j+1})^tau x_(j t) <= 1 forall tau=1,...T
  for (size_t tau = 0; tau < big_t; tau++) {
    index = 0;
    c_size = 0;
    offset_j = 0;
    for (size_t j = 0; j < n; j++) {
      int max = tau - instance->processing_times[j] + 1;
      if (max < 0)
        max = 0;
      // There are not enough time slots to process the entire job
      if (big_t - tau < instance->processing_times[j] || tau - max + 1 == 0) {
        offset_j += big_t - instance->processing_times[j] + 1;
        continue;
      }
      c_size += tau - max + 1;

      if ((result = realloc_constr(&c_index, &c_vals, c_size)) != 0)
        return result;
      for (size_t t = max; t <= tau; t++) {
        c_index[index] = offset_j + t;
        c_vals[index] = 1;
        index += 1;
      }
      offset_j += big_t - instance->processing_times[j] + 1;
    }

    if (c_size == 0)
      continue;

    if ((result = GRBaddconstr(instance->model, c_size, c_index, c_vals,
                               GRB_LESS_EQUAL, 1, NULL)) != 0) {
      perror(formatted_string("Constraint tau = %ld", tau));
      log_error(sim, result, "GRBaddconstr");
      c_print(c_size, c_index, c_vals, names);
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
      perror(formatted_string("Constraint j = %ld", j));
      log_error(sim, result, "GRBaddconstr");
      c_print(c_size, c_index, c_vals, names);
      return result;
    }
    offset_j += big_t - instance->processing_times[j] + 1;
  }

  for (size_t i = 0; i < size; i++) {
    free(names[i]);
    names[i] = NULL;
  }
  free(names);
  names = NULL;
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

int model_heuristics_create(simulation_t *sim, instance_t *instance,
                            int *heuristic_value) {
  int result = 0;
  int n = instance->number_of_jobs;
  // Array to keep track of index changes when sorting
  int *indexes = malloc(sizeof(*indexes) * n);
  if (indexes == NULL) {
    perror("Could not allocate memory for indexes");
    return -1;
  }
  for (size_t i = 0; i < n; i++) {
    indexes[i] = (int)i;
  }
  sort(instance, indexes);
  int *c_hs = malloc(sizeof(*c_hs) * n);
  if (c_hs == NULL) {
    perror("Could not allocate memory for C_js");
    return -1;
  }
  c_hs[0] = instance->release_dates[0] + instance->processing_times[0];
  *heuristic_value = c_hs[0];
  for (size_t i = 1; i < n; i++) {
    // Current job start when previous job finished
    int s_h = c_hs[i - 1];
    // The next job is not released -> adding idle time until it's released
    if (s_h < instance->release_dates[i])
      s_h += instance->release_dates[i] - c_hs[i - 1];
    int c_h = s_h + instance->processing_times[i];

    c_hs[i] = c_h;
    *heuristic_value += c_h;
  }
  if ((result = model_positional_create(sim, instance)) != 0) {
    perror("Could not create positional model for heuristic case");
    return result;
  }

  // Initial solution value for C_[h] variables
  for (size_t i = 0; i < n; i++) {
    if ((result = GRBsetdblattrelement(instance->model, "Start", i,
                                       (double)c_hs[i])) != 0) {
      log_error(sim, result, "GRBsetdblattrelement(\"Start\")");
      return result;
    }
  }
  for (size_t i = 0; i < n; i++) {
    int index = n + indexes[i] * instance->number_of_jobs + i;
    if ((result = GRBsetdblattrelement(instance->model, "Start", index,
                                       (double)1)) != 0) {
      log_error(sim, result, "GRBsetdblattrelement(\"Start\")");
      return result;
    }
  }
  return result;
}

void c_print(int size, int *index, double *vals, char **names) {
  for (size_t i = 0; i < size; i++) {
    char *sign = "+";
    if (vals[i] < 0)
      sign = "";
    char *multiplier = "";
    printf("%s%.2f %s ", sign, vals[i], names[index[i]]);

    if (i == size - 1)
      printf("");
  }
  printf("\n");
}

int realloc_constr(int **constr_index, double **constr_vals, size_t size) {
  *constr_index = realloc(*constr_index, sizeof(**constr_index) * size);
  if (*constr_index == NULL) {
    perror("Could not realloc constr_index");
    return -1;
  }

  *constr_vals = realloc(*constr_vals, sizeof(**constr_vals) * size);
  if (*constr_vals == NULL) {
    perror("Could not realloc constr_vals");
    return -1;
  }

  return 0;
}
