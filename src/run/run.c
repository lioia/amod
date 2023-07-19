#include "run.h"
#include "../utils/csv.h"
#include "../utils/strings.h"
#include "gurobi_c.h"
#include "model/model.h"
#include <stdio.h>
#include <stdlib.h>

int run(const char *filename) {
  int result = 0;

  vector_t *instances = vector_init();
  if (instances == NULL)
    return -1;

  if ((result = load_csv(filename, instances)) != 0)
    return result;

  simulation_t *sim = environment_init(instances);
  if (sim == NULL)
    return -1;

  FILE *error_fp = fopen("error.csv", "w");
  if (error_fp == NULL) {
    perror("Could not open error.csv");
    return -1;
  }
  fprintf(error_fp, "Solver,Instance,Function\n");
  FILE *solution_fp = fopen("solution.csv", "w");
  if (solution_fp == NULL) {
    perror("Could not open solution.csv");
    return -1;
  }
  fprintf(solution_fp, "Solver,Instance,Status,Runtime\n");

  // TODO: change TimeIndexed to Heuristics when implemented
  for (solver_t solver = Precedence; solver <= TimeIndexed; solver++) {
    printf("---------------------------\n");
    printf("\tSolver: %d\n", solver);
    for (size_t i = 0; i < sim->instances->length; i++) {
      if ((result = model_init(sim, i, solver)) != 0) {
        fprintf(error_fp, "%d,%ld,Init\n", solver, i + 1);
        continue;
      }

      solution_t *solution = model_optimize(sim, i, solver);
      if (solution == NULL) {
        fprintf(error_fp, "%d,%ld,Optimize\n", solver, i + 1);
        continue;
      }
      fprintf(solution_fp, "%d,%ld,%d,%.2f\n", solver, i + 1, solution->status,
              solution->runtime);
    }
  }

  if ((result = simulation_free(sim)) != 0)
    return result;

  fclose(error_fp);
  fclose(solution_fp);
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

  if ((result = GRBsetstrparam(sim->env, "LogFile", "model.log")) != 0) {
    log_error(sim, result, "GRBsetstrparam(\"LogFile\")");
    return NULL;
  }

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
