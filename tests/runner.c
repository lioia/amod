#include <stdio.h>
#include <stdlib.h>

#include "../src/run/model/model.h"
#include "../src/run/run.h"
#include "../src/utils/entities.h"
#include "gurobi_c.h"

solution_t *model_precedence_test(simulation_t *simulation);
solution_t *model_positional_test(simulation_t *simulation);
solution_t *model_timeindexed_test(simulation_t *simulation);
int save_solution(solution_t *solution);

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
  printf("---------------------------\n");
  printf("Model Precedence Test\n");
  solution_t *solution = model_precedence_test(sim);
  if (solution == NULL)
    fprintf(stderr, "Model Precedence Test failed\n");
  else
    save_solution(solution);
  printf("---------------------------\n");
  printf("\nModel Positional Test\n");
  solution = model_positional_test(sim);
  if (solution == NULL)
    fprintf(stderr, "Model Positional Test failed\n");
  else
    save_solution(solution);
  printf("---------------------------\n");
  printf("\nModel Time Indexed Test\n");
  solution = model_timeindexed_test(sim);
  if (solution == NULL)
    fprintf(stderr, "Model Time Indexed Test failed\n");
  else
    save_solution(solution);
  printf("---------------------------\n");

  // Teardown
  if ((result = simulation_free(sim)) != 0)
    return result;
  return result;
}

int save_solution(solution_t *solution) {
  int result = 0;
  printf("Found solution:\n");
  printf("\tStatus: %d\n", solution->status);
  printf("\tObjective Value: %f\n", solution->objective_value);
  printf("\tObjective Solution: (");
  for (size_t i = 0; i < solution->size; i++) {
    printf("%f", solution->values[i]);
    if (i != solution->size - 1)
      printf(", ");
  }
  printf(")\n");
  size_t len = snprintf(NULL, 0, "%d.json", solution->solver) + 1;
  char *name = malloc(sizeof(*name) * len);
  if (name == NULL) {
    name = "unknown.json";
  } else {
    sprintf(name, "%d.json", solution->solver);
    name[len - 1] = '\0';
  }
  if ((result = GRBwrite(solution->model, name)) != 0) {
    fprintf(stderr, "Could not write %s\n", name);
    return result;
  }

  return 0;
}

solution_t *model_precedence_test(simulation_t *simulation) {
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, Precedence) != 0) {
    fprintf(stderr, "Could not init model\n");
    return NULL;
  }

  if (GRBwrite(instance->model, "precedence.lp") != 0) {
    fprintf(stderr, "Could not write precedence.lp\n");
    return NULL;
  }
  return model_optimize(simulation, 0, Precedence);
}

solution_t *model_positional_test(simulation_t *simulation) {
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, Positional) != 0) {
    fprintf(stderr, "Could not init model\n");
    return NULL;
  }

  if (GRBwrite(instance->model, "positional.lp") != 0) {
    fprintf(stderr, "Could not write positional.lp\n");
    return NULL;
  }
  return model_optimize(simulation, 0, Positional);
}

solution_t *model_timeindexed_test(simulation_t *simulation) {
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, TimeIndexed) != 0) {
    fprintf(stderr, "Could not init model\n");
    return NULL;
  }

  if (GRBwrite(instance->model, "timeindexed.lp") != 0) {
    fprintf(stderr, "Could not write timeindexed.lp\n");
    return NULL;
  }

  return model_optimize(simulation, 0, TimeIndexed);
}
