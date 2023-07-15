#pragma once

#include "../utils/entities.h"

int run(int workers, const char *filename);

simulation_t *environment_init(vector_t *instances);
int simulation_free(simulation_t *simulation);
int handle_instances(solver_t solver, simulation_t *sim);

void report_error(simulation_t *simulation, const char *error, int result);
