#include <stdio.h>

#include "../src/run/model/model.h"
#include "../src/run/run.h"
#include "../src/utils/entities.h"
#include "gurobi_c.h"

int model_precedence_test(simulation_t *simulation);
int model_positional_test(simulation_t *simulation);

int main(void) {
  int result = 0;
  // Setup
  int processing_times[3] = {3, 1, 4};
  int release_dates[3] = {5, 0, 2};
  instance_t dummy_instance = {.number_of_jobs = 3,
                               .processing_times = processing_times,
                               .release_dates = release_dates};

  vector_t *instances = vector_init();
  if (instances == NULL)
    return -1;
  if ((result = vector_add(instances, &dummy_instance)) != 0)
    return result;
  simulation_t *sim = environment_init(instances);

  // Execute tests
  printf("---------------------------\n");
  printf("Model Precedence Test\n");
  if ((result = model_precedence_test(sim)) != 0)
    fprintf(stderr, "Model Precedence Test failed\n");
  printf("---------------------------\n");
  printf("Model Positional Test\n");
  if ((result = model_positional_test(sim)) != 0)
    fprintf(stderr, "Model Positional Test failed\n");
  printf("---------------------------\n");

  // Teardown
  if ((result = simulation_free(sim)) != 0)
    return result;
  return result;
}

int model_precedence_test(simulation_t *simulation) {
  int result = 0;
  instance_t *instance = simulation->instances->values[0];
  if ((result = model_init(simulation, 0, Precedence)) != 0)
    return result;

  if ((result = GRBwrite(instance->model, "precedence.lp")) != 0) {
    fprintf(stderr, "Could not write precedence.lp\n");
    return result;
  }

  solution_t *solution = model_optimize(simulation, 0, Positional);

  if (solution == NULL)
    return -1;

  printf("Found solution:\n");
  printf("\tStatus: %d\n", solution->status);
  printf("\tObjective Value: %f\n", solution->objective_value);
  printf("\tObjective Solution: (");
  for (size_t i = 0; i < instance->number_of_jobs; i++) {
    printf("%f", solution->values[i]);
    if (i != instance->number_of_jobs - 1)
      printf(", ");
  }
  printf(")\n");
  if ((result = GRBwrite(instance->model, "precedence.json")) != 0) {
    fprintf(stderr, "Could not write precedence.json\n");
    return result;
  }

  return result;
}

int model_positional_test(simulation_t *simulation) {
  int result = 0;
  instance_t *instance = simulation->instances->values[0];
  if ((result = model_init(simulation, 0, Positional)) != 0)
    return result;

  if ((result = GRBwrite(instance->model, "positional.lp")) != 0) {
    fprintf(stderr, "Could not write positional.lp\n");
    return result;
  }

  solution_t *solution = model_optimize(simulation, 0, Positional);

  if (solution == NULL)
    return -1;

  printf("Found solution:\n");
  printf("\tStatus: %d\n", solution->status);
  printf("\tObjective Value: %f\n", solution->objective_value);
  printf("\tObjective Solution: (");
  for (size_t i = 0; i < instance->number_of_jobs; i++) {
    printf("%f", solution->values[i]);
    if (i != instance->number_of_jobs - 1)
      printf(", ");
  }
  printf(")\n");
  if ((result = GRBwrite(instance->model, "positional.json")) != 0) {
    fprintf(stderr, "Could not write positional.json\n");
    return result;
  }

  return result;
}
