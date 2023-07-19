#include "run.h"
#include "../utils/agnostic_io.h"
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
  fprintf(sol_fp, "Solver,Instance,Status,Runtime\n");

  simulation_t *sim = environment_init(instances);
  if (sim == NULL)
    return -1;

  // TODO: change TimeIndexed to Heuristics when implemented
  for (solver_t solver = Precedence; solver <= TimeIndexed; solver++) {
    if (create_folder(formatted_string("output/%d", solver)) != 0) {
      perror(formatted_string("Could not create folder output/%d", solver));
      continue;
    }
    for (size_t i = 0; i < sim->instances->length; i++) {
      instance_t *instance = sim->instances->values[i];
      if ((result = model_init(sim, i, solver)) != 0) {
        fprintf(error_fp, "%d,%ld,Init\n", solver, i + 1);
        continue;
      }

      char *lp_name = formatted_string("output/%d/%ld.lp", solver, i);
      if (lp_name == NULL)
        lp_name = "output/unknown.lp";
      if ((result = GRBwrite(instance->model, lp_name)) != 0)
        log_error(sim, result, "GRBwrite lp");

      char *mps_name = formatted_string("output/%d/%ld.mps", solver, i);
      if (lp_name == NULL)
        lp_name = "output/unknown.mps";
      if ((result = GRBwrite(instance->model, lp_name)) != 0)
        log_error(sim, result, "GRBwrite mps");

      solution_t *solution = model_optimize(sim, i, solver);
      if (solution == NULL) {
        fprintf(error_fp, "%d,%ld,Optimize\n", solver, i + 1);
        continue;
      }
      fprintf(sol_fp, "%d,%ld,%d,%.2f\n", solver, i + 1, solution->status,
              solution->runtime);

      char *json_name = formatted_string("output/%d/%ld.json", solver, i);
      if (json_name == NULL)
        json_name = "output/unknown.json";
      if ((result = GRBwrite(instance->model, json_name)) != 0)
        log_error(sim, result, "GRBwrite json");

      char *sol_name = formatted_string("output/%d/%ld.sol", solver, i);
      if (sol_name == NULL)
        json_name = "output/unknown.sol";
      if ((result = GRBwrite(instance->model, sol_name)) != 0)
        log_error(sim, result, "GRBwrite sol");
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
