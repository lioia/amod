#include "run.h"
#include "../utils/csv.h"
#include "../utils/utils.h"
#include "gurobi_c.h"
#include "model/model.h"
#include <stdio.h>
#include <stdlib.h>

void save_model(simulation_t *sim, size_t i, solver_t solver, char *format);

int run(const char *filename) {
  int result = 0;

  vector_t *instances = vector_init();
  if (instances == NULL)
    return -1;

  if ((result = load_csv(filename, instances)) != 0)
    return result;

  if ((result = create_folder("output")) != 0) {
    perror("Could not create folder output");
    return result;
  }
  FILE *error_fp = fopen("output/error.csv", "w");
  if (error_fp == NULL) {
    perror("Could not open error.csv");
    return -1;
  }
  fprintf(error_fp, "Solver,Instance,Function\n");
  FILE *sol_fp = fopen("output/solution.csv", "w");
  if (sol_fp == NULL) {
    perror("Could not open solution.csv");
    return -1;
  }
  fprintf(sol_fp, "Solver,Instance,Status,Runtime,Solution,Heuristic\n");

  simulation_t *sim = environment_init(instances);
  if (sim == NULL)
    return -1;

  for (solver_t solver = Precedence; solver <= Heuristics; solver++) {
    if (create_folder(formatted_string("output/%d", solver)) != 0) {
      perror(formatted_string("Could not create folder output/%d", solver));
      continue;
    }
    for (size_t i = 0; i < sim->instances->length; i++) {
      int heuristic_value = -1;
      instance_t *instance = sim->instances->values[i];
      if ((result = model_init(sim, i, solver, &heuristic_value)) != 0) {
        fprintf(error_fp, "%d,%ld,Init\n", solver, i + 1);
        continue;
      }

      save_model(sim, i, solver, "lp");
      save_model(sim, i, solver, "mps");

      solution_t *solution = model_optimize(sim, i, solver);
      if (solution == NULL) {
        fprintf(error_fp, "%d,%ld,Optimize\n", solver, i + 1);
        continue;
      }
      solution->heuristic_value = heuristic_value;
      fprintf(sol_fp, "%d,%ld,%d,%.2f,%.2f,%.2f\n", solver, i + 1,
              solution->status, solution->runtime, solution->objective_value,
              solution->heuristic_value);
      save_model(sim, i, solver, "json");
      save_model(sim, i, solver, "sol");
    }
  }

  if ((result = simulation_free(sim)) != 0)
    return result;

  fclose(error_fp);
  fclose(sol_fp);
  return result;
}

simulation_t *environment_init(vector_t *instances) {
  int result = 0;
  printf("Initializing simulation...");
  simulation_t *sim = malloc(sizeof(*sim));
  if (sim == NULL) {
    perror("Could not allocate memory for simulation");
    return NULL;
  }
  sim->instances = instances;

  if ((result = GRBemptyenv(&sim->env)) != 0) {
    log_error(sim, result, "GRBemptyenv");
    return NULL;
  }

  // if ((result = GRBsetstrparam(sim->env, "LogFile", "model.log")) != 0) {
  //   log_error(sim, result, "GRBsetstrparam(\"LogFile\")");
  //   return NULL;
  // }

  if ((result = GRBstartenv(sim->env)) != 0) {
    log_error(sim, result, "GRBstartenv");
    return NULL;
  }

  printf("Environment started correctly\n");

  return sim;
}

int simulation_free(simulation_t *sim) {
  for (size_t i = 0; i < sim->instances->length; i++) {
    instance_t *instance = sim->instances->values[i];
    GRBfreemodel(instance->model);
    instance->model = NULL;
  }
  vector_free(sim->instances);
  sim->instances = NULL;
  GRBfreeenv(sim->env);
  sim->env = NULL;
  free(sim);
  sim = NULL;
  return 0;
}

void save_model(simulation_t *sim, size_t i, solver_t solver, char *format) {
  int result = 0;
  instance_t *instance = sim->instances->values[i];
  char *name = formatted_string("output/%d/%ld.%s", solver, i, format);
  if (name != NULL) {
    if ((result = GRBwrite(instance->model, name)) != 0)
      log_error(sim, result, "GRBwrite");
  }
}
