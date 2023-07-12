#include "run.h"
#include "../utils/csv.h"
#include "gurobi_c.h"
#include <stdio.h>
#include <stdlib.h>

int run(int workers, const char *filename) {
  int result = 0;

  vector_t instances;
  if ((result = vector_init(&instances)) != 0)
    return result;

  if ((result = load_csv(filename, &instances)) != 0)
    return result;

  simulation_t sim = {.instances = &instances};
  if ((result = init_simulation(&sim)) != 0)
    return result;
  return result;
}

int init_simulation(simulation_t *sim) {
  int result = 0;
  printf("Initializing simulation...\n");

  if ((result = GRBemptyenv(&sim->gurobi_env)) != 0) {
    report_gurobi_error(sim, "Could not create empty environment", result);
    return result;
  }

  if ((result = GRBsetstrparam(sim->gurobi_env, "LogFile", "model.log")) != 0) {
    report_gurobi_error(sim, "Could not set LogFile parameter", result);
    return result;
  }

  if ((result = GRBstartenv(sim->gurobi_env)) != 0) {
    report_gurobi_error(sim, "Could not start environment", result);
    return result;
  }

  printf("Environment started correctly");

  return result;
}

int free_simulation(simulation_t *sim) {
  int result = 0;
  if (sim->instances != NULL)
    result = free_model(sim);
  GRBfreeenv(sim->gurobi_env);
  free(sim);
  sim = NULL;
  return result;
}

int create_model(simulation_t *sim, const char *name) {
  int result = GRBnewmodel(sim->gurobi_env, &sim->gurobi_model, name, 0, NULL,
                           NULL, NULL, NULL, NULL);
  if (result)
    return result;

  // TODO: initialize model

  return 0;
}

int free_model(simulation_t *sim) {
  int result = GRBfreemodel(sim->gurobi_model);
  if (result)
    return result;

  vector_free(sim->instances);

  return 0;
}

void report_gurobi_error(simulation_t *sim, const char *error, int result) {
  const char *error_msg = GRBgeterrormsg(sim->gurobi_env);
  fprintf(stderr, "%s (error code %d): %s\n", error, result, error_msg);
}
