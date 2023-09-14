#include <stdio.h>
#include <stdlib.h>

#include "../src/run/model/model.h"
#include "../src/run/run.h"
#include "../src/utils/entities.h"
#include "gurobi_c.h"

solution_t *model_precedence_test(simulation_t *simulation);
solution_t *model_positional_test(simulation_t *simulation);
solution_t *model_timeindexed_test(simulation_t *simulation);
solution_t *model_heuristics_precedence_test(simulation_t *simulation);
solution_t *model_heuristics_positional_test(simulation_t *simulation);

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
  dummy_instance->model = NULL;

  vector_t *instances = vector_init();
  if (instances == NULL)
    return -1;
  if ((result = vector_add(instances, (void **)&dummy_instance)) != 0)
    return result;
  simulation_t *sim = environment_init(instances);
  solution_t *solution;

  // Execute tests
  printf("---------------------------\n");
  printf("Model Precedence Test");
  solution = model_precedence_test(sim);
  if (solution == NULL) {
    result = -1;
    perror("Model Precedence Test failed");
  }
  printf("---------------------------\n");
  printf("Model Positional Test\n");
  solution = model_positional_test(sim);
  if (solution == NULL) {
    result = -1;
    perror("Model Positional Test failed");
  }
  printf("---------------------------\n");
  printf("Model Time Indexed Test\n");
  solution = model_timeindexed_test(sim);
  if (solution == NULL) {
    result = -1;
    perror("Model Time Indexed Test failed");
  }
  printf("---------------------------\n");
  printf("Model Heuristics Predecence Test\n");
  solution = model_heuristics_precedence_test(sim);
  if (solution == NULL) {
    result = -1;
    perror("Model Heuristics Precedence Test failed");
  }
  printf("---------------------------\n");
  printf("Model Heuristics Positional Test\n");
  solution = model_heuristics_positional_test(sim);
  if (solution == NULL) {
    result = -1;
    perror("Model Heuristics Positional Test failed");
  }
  printf("---------------------------\n");

  // Teardown
  if (solution != NULL) {
    free(solution->values);
    solution->values = NULL;
    free(solution);
    solution = NULL;
  }
  if ((result = simulation_free(sim)) != 0)
    return result;
  return result;
}

solution_t *model_precedence_test(simulation_t *simulation) {
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, Precedence, NULL) != 0) {
    perror("Could not init model");
    return NULL;
  }

  if (GRBwrite(instance->model, "output/precedence.lp") != 0) {
    perror("Could not write precedence.lp");
    return NULL;
  }
  return model_optimize(simulation, 0, Precedence);
}

solution_t *model_positional_test(simulation_t *simulation) {
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, Positional, NULL) != 0) {
    perror("Could not init model");
    return NULL;
  }

  if (GRBwrite(instance->model, "output/positional.lp") != 0) {
    perror("Could not write positional.lp");
    return NULL;
  }
  return model_optimize(simulation, 0, Positional);
}

solution_t *model_timeindexed_test(simulation_t *simulation) {
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, TimeIndexed, NULL) != 0) {
    perror("Could not init model");
    return NULL;
  }

  if (GRBwrite(instance->model, "output/timeindexed.lp") != 0) {
    perror("Could not write timeindexed.lp");
    return NULL;
  }

  return model_optimize(simulation, 0, TimeIndexed);
}

solution_t *model_heuristics_precedence_test(simulation_t *simulation) {
  int heuristic_value;
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, Heuristics_Precedence, &heuristic_value) != 0) {
    perror("Could not init model");
    return NULL;
  }

  if (GRBwrite(instance->model, "output/heuristic_precedence.lp") != 0) {
    perror("Could not write heuristic_precedence.lp");
    return NULL;
  }
  solution_t *solution = model_optimize(simulation, 0, Positional);
  if (solution != NULL)
    solution->heuristic_value = heuristic_value;
  return solution;
}

solution_t *model_heuristics_positional_test(simulation_t *simulation) {
  int heuristic_value;
  instance_t *instance = simulation->instances->values[0];
  if (model_init(simulation, 0, Heuristics_Positional, &heuristic_value) != 0) {
    perror("Could not init model");
    return NULL;
  }

  if (GRBwrite(instance->model, "output/heuristic_positional.lp") != 0) {
    perror("Could not write heuristic_positional.lp");
    return NULL;
  }
  solution_t *solution = model_optimize(simulation, 0, Positional);
  if (solution != NULL)
    solution->heuristic_value = heuristic_value;
  return solution;
}
