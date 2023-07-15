#include "run.h"
#include "../utils/csv.h"
#include "gurobi_c.h"
#include "model/model.h"
#include <stdio.h>
#include <stdlib.h>

int run(int workers, const char *filename) {
  int result = 0;

  vector_t *instances = vector_init();
  if (instances == NULL)
    return -1;

  if ((result = load_csv(filename, instances)) != 0)
    return result;

  simulation_t *sim = environment_init(instances);
  if (sim == NULL)
    return -1;

  // TODO: change TimeIndexed to Heuristics when implemented
  for (solver_t solver = Precedence; solver <= TimeIndexed; solver++) {
    // TODO: create thread to execute every solver
    handle_instances(solver, sim);
  }

  if ((result = simulation_free(sim)) != 0)
    return result;
  return result;
}

simulation_t *environment_init(vector_t *instances) {
  int result = 0;
  printf("Initializing simulation...\n");
  simulation_t *sim = malloc(sizeof(*sim));
  if (sim == NULL) {
    fprintf(stderr, "Could not allocate memory for simulation\n");
    return NULL;
  }
  sim->instances = instances;

  if ((result = GRBemptyenv(&sim->env)) != 0) {
    report_error(sim, "Could not create empty environment", result);
    return NULL;
  }

  if ((result = GRBsetstrparam(sim->env, "LogFile", "model.log")) != 0) {
    report_error(sim, "Could not set LogFile parameter", result);
    return NULL;
  }

  if ((result = GRBstartenv(sim->env)) != 0) {
    report_error(sim, "Could not start environment", result);
    return NULL;
  }

  printf("Environment started correctly\n");

  return sim;
}

int simulation_free(simulation_t *sim) {
  vector_free(sim->instances);
  sim->instances = NULL;
  GRBfreeenv(sim->env);
  free(sim);
  sim = NULL;
  return 0;
}

int handle_instances(solver_t solver, simulation_t *sim) {
  int result = 0;
  for (size_t i = 0; i < sim->instances->length; i++) {
    if ((result = model_init(sim, i, solver)) != 0)
      return result;
  }
  return 0;
}

void report_error(simulation_t *sim, const char *error, int result) {
  const char *error_msg = GRBgeterrormsg(sim->env);
  fprintf(stderr, "%s (error code %d): %s\n", error, result, error_msg);
}
