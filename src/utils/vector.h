#pragma once

#define VECTOR_DEFAULT_SIZE 32

typedef struct {
  int allocated_size;
  int length; // Number of elements in the vector
  void **values;
} vector_t;

int vector_init(vector_t *vector);
int vector_add(vector_t *vector, void *value);
int vector_free(vector_t *vector);
