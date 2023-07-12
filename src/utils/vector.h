#pragma once

#define VECTOR_DEFAULT_SIZE 32

typedef struct {
  int allocated_size;
  int length; // Number of elements in the vector
  void **values;
} vector_t;

// Initialize a vector (`vector` already exists)
int vector_init(vector_t *vector);
// Add `value` to `vector` (resizing if necessary)
int vector_add(vector_t *vector, void *value);
// Reduce vector size to be exactly its length
int vector_refit(vector_t *vector);
// Clean every value saved in vector and vector itself
int vector_free(vector_t *vector);
