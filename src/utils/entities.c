#include "entities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

vector_t *vector_init() {
  vector_t *vector = malloc(sizeof(*vector));
  if (vector == NULL) {
    fprintf(stderr, "Could not allocate memory for vector\n");
    return NULL;
  }
  void **values = malloc(sizeof(*values) * VECTOR_DEFAULT_SIZE);
  if (values == NULL) {
    fprintf(stderr, "Could not allocate memory for vector values\n");
    return NULL;
  }
  memset(values, 0, sizeof(*values) * VECTOR_DEFAULT_SIZE);

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
      fprintf(stderr, "Could not reallocate vector values\n");
      return -1;
    }

    vector->allocated_length = new_size;
  }

  vector->values[vector->length++] = *value;
  return 0;
}

int vector_free(vector_t *vector) {
  free(vector->values);
  free(vector);
  return 0;
}

int vector_refit(vector_t *vector) {
  if (vector->length == vector->allocated_length)
    return 0;
  vector->values =
      realloc(vector->values, sizeof(*vector->values) * vector->length);
  if (*vector->values == NULL) {
    fprintf(stderr, "Could not reallocate memory for values\n");
    return -1;
  }
  vector->allocated_length = vector->length;
  return 0;
}

int vector_reset(vector_t *vector) {
  if (vector->allocated_length != VECTOR_DEFAULT_SIZE) {
    vector->values =
        realloc(vector->values, sizeof(*vector->values) * VECTOR_DEFAULT_SIZE);
    if (*vector->values == NULL) {
      fprintf(stderr, "Could not rellocate memory for values when resetting\n");
      return -1;
    }
  }
  memset(vector->values, 0, sizeof(*vector->values) * VECTOR_DEFAULT_SIZE);
  vector->length = 0;
  vector->allocated_length = VECTOR_DEFAULT_SIZE;
  return 0;
}
