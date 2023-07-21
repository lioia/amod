#include "entities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

vector_t *vector_init() {
  vector_t *vector = malloc(sizeof(*vector));
  if (vector == NULL) {
    perror("Could not allocate memory for vector");
    return NULL;
  }
  void **values = malloc(sizeof(*values) * VECTOR_DEFAULT_SIZE);
  if (values == NULL) {
    perror("Could not allocate memory for vector values");
    return NULL;
  }
  for (size_t i = 0; i < VECTOR_DEFAULT_SIZE; i++) {
    values[i] = NULL;
  }

  vector->allocated_length = VECTOR_DEFAULT_SIZE;
  vector->length = 0;
  vector->values = values;

  return vector;
}

int vector_add(vector_t *vector, void **value) {
  // Need to reallocate
  if (vector->length == vector->allocated_length) {
    int new_size = vector->allocated_length + VECTOR_DEFAULT_SIZE;

    vector->values = realloc(vector->values, sizeof(*value) * new_size);
    if (vector->values == NULL) {
      perror("Could not reallocate vector values");
      return -1;
    }

    vector->allocated_length = new_size;
    for (size_t i = vector->length; i < VECTOR_DEFAULT_SIZE; i++) {
      vector->values[i] = NULL;
    }
  }

  vector->values[vector->length++] = *value;
  return 0;
}

int vector_free(vector_t *vector) {
  for (size_t i = 0; i < vector->length; i++) {
    free(vector->values[i]);
    vector->values[i] = NULL;
  }
  free(vector->values);
  free(vector);
  return 0;
}

int vector_refit(vector_t *vector) {
  if (vector->length == vector->allocated_length)
    return 0;
  vector->values =
      realloc(vector->values, sizeof(*vector->values) * vector->length);
  if (vector->values == NULL) {
    perror("Could not reallocate memory for values");
    return -1;
  }
  vector->allocated_length = vector->length;
  return 0;
}
