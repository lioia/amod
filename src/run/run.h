#pragma once

#include "../utils/entities.h"

int run(int workers, const char *filename);

int init_simulation(simulation_t *simulation);
int free_simulation(simulation_t *simulation);

int create_model(simulation_t *simulation, const char *name);
int free_model(simulation_t *simulation);

void report_gurobi_error(simulation_t *simulation, const char *error,
                         int result);
