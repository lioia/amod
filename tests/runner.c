#include <stdio.h>
#include <stdlib.h>

#include "../src/run/model/model.h"
#include "../src/run/run.h"
#include "../src/utils/entities.h"
#include "gurobi_c.h"

solution_t *model_precedence_test(simulation_t *simulation);
solution_t *model_positional_test(simulation_t *simulation);
solution_t *model_timeindexed_test(simulation_t *simulation);

int main(void) {
  int result = 0;
  // Setup
  int processing_times[3] = {3, 1, 4};
  int release_dates[3] = {5, 0, 2};

  instance_t *dummy_instance = malloc(sizeof(*dummy_instance));
  if (dummy_instance == NULL)
    return -1;
  dummy_instance->number_of_jobs = 3;
  dummy_instance->processing_times = processing_times;
  dummy_instance->release_dates = release_dates;

  vector_t *instances = vector_init();
  if (instances == NULL)
    return -1;
  if ((result = vector_add(instances, (void **)&dummy_instance)) != 0)
    return result;
  simulation_t *sim = environment_init(instances);

  // Execute tests
  // printf("---------------------------");
  // printf("Model Precedence Test");
  // solution = model_precedence_test(sim);
  // if (solution == NULL)
  //   perror("Model Precedence Test failed");
  // printf("---------------------------");
  // printf("Model Positional Test\n");
  // solution_t *solution = model_positional_test(sim);
  // if (solution == NULL)
  //   perror("Model Positional Test failed");
  printf("---------------------------");
  printf("Model Time Indexed Test\n");
  solution_t *solution = model_timeindexed_test(sim);
  if (solution == NULL)
    perror("Model Time Indexed Test failed");
  printf("---------------------------");

  // Teardown
  free(solution->values);
  solution->values = NULL;
  free(solution);
  solution = NULL;
  if ((result = simulation_free(sim)) != 0)
    return result;
  return result;
}

solution_t *model_precedence_test(simulation_t *simulation) {
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, Precedence) != 0) {
    perror("Could not init model");
    return NULL;
  }

  if (GRBwrite(instance->model, "precedence.lp") != 0) {
    perror("Could not write precedence.lp");
    return NULL;
  }
  return model_optimize(simulation, 0, Precedence);
}

solution_t *model_positional_test(simulation_t *simulation) {
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, Positional) != 0) {
    perror("Could not init model");
    return NULL;
  }

  if (GRBwrite(instance->model, "positional.lp") != 0) {
    perror("Could not write positional.lp");
    return NULL;
  }
  return model_optimize(simulation, 0, Positional);
}

solution_t *model_timeindexed_test(simulation_t *simulation) {
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, TimeIndexed) != 0) {
    perror("Could not init model");
    return NULL;
  }

  if (GRBwrite(instance->model, "timeindexed.lp") != 0) {
    perror("Could not write timeindexed.lp");
    return NULL;
  }

  return model_optimize(simulation, 0, TimeIndexed);
}
