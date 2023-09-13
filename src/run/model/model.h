#pragma once

#include "../../utils/entities.h"

#define TIME_LIMIT 60.0 * 5 // 5 minutes

typedef struct {
  int index;
  double val;
} tuple_t;

int model_init(simulation_t *simulation, int instance_number, solver_t solver,
               int *heuristic_value);
solution_t *model_optimize(simulation_t *simulation, int instance_number,
                           solver_t solver);
int model_precedence_create(simulation_t *simulation, instance_t *instance);
int model_positional_create(simulation_t *simulation, instance_t *instance);
int model_time_indexed_create(simulation_t *simulation, instance_t *instance);
int model_heuristics_create(simulation_t *simulation, instance_t *instance,
                            int *heuristic_value);
