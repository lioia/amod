#pragma once

#include "../utils/entities.h"

int run(const char *filename);

simulation_t *environment_init(vector_t *instances);
int simulation_free(simulation_t *simulation);
