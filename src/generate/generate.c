#include "generate.h"
#include <stdio.h>
#include <time.h>

int generate(const char *filename) {
  FILE *fp;
  if ((fp = fopen(filename, "w")) == NULL)
    return -1;
  fprintf(fp, "Instance,ProcessingTime,ReleaseDate\n");

  plant_seeds(-1);

  int instance_count = 0;

  for (size_t i = 0; i < 3; i++) {
    int number_of_jobs_ul = NUMBER_OF_JOBS_UL[i];
    for (size_t j = 0; j < 3; j++) {
      int processing_time_ul = PROCESSING_TIMES_UL[j];
      for (size_t k = 0; k < 3; k++) {
        int release_date_ul = RELEASE_DATES_UL[k];
        for (size_t l = 0; l < NUMBER_OF_INSTANCES; l++) {
          instance_count += 1;
          select_stream(0);
          int number_of_jobs = uniform(1, number_of_jobs_ul);
          for (size_t job = 0; job < number_of_jobs; job++) {
            select_stream(1);
            int processing_time = uniform(1, processing_time_ul);
            select_stream(2);
            int release_date = uniform(1, release_date_ul);
            fprintf(fp, "%d,%d,%d\n", instance_count, processing_time,
                    release_date);
          }
        }
      }
    }
  }
  fclose(fp);
  printf("Generated %d instances\n", instance_count);
  return 0;
}

/*
 * From Discrete Event Simulation, Leemis Park
 */

// Return a uniformly distributed random number in [a, b]
int uniform(int a, int b) { return a + ((b - a + 1) * rng()); }

// Returns a pseudo-random real number uniformly distributed between 0 and 1
double rng(void) {
  const long Q = MODULUS / MULTIPLIER;
  const long R = MODULUS % MULTIPLIER;
  long t;

  t = MULTIPLIER * (seed[stream] % Q) - R * (seed[stream] / Q);
  if (t > 0)
    seed[stream] = t;
  else
    seed[stream] = t + MODULUS;
  return (double)seed[stream] / MODULUS;
}

/* Use this function to set the state of all the random number generator
 * streams by "planting" a sequence of states (seeds), one per stream,
 * with all states dictated by the state of the default stream.
 * The sequence of planted states is separated one from the next by
 * 8,367,782 calls to Random().
 */
void plant_seeds(long x) {
  const long Q = MODULUS / A256;
  const long R = MODULUS % A256;
  int j;
  int s;
  initialized = 1;
  s = stream;       /* remember the current stream */
  select_stream(0); /* change to stream 0          */
  put_seed(x);      /* set seed[0]                 */
  stream = s;       /* reset the current stream    */
  for (j = 1; j < STREAMS; j++) {
    x = A256 * (seed[j - 1] % Q) - R * (seed[j - 1] / Q);
    if (x > 0)
      seed[j] = x;
    else
      seed[j] = x + MODULUS;
  }
}

/*
 * Use this function to set the state of the current random number
 * generator stream according to the following conventions:
 *    if x > 0 then x is the state (unless too large)
 *    if x < 0 then the state is obtained from the system clock
 *    if x = 0 then the state is to be supplied interactively
 */
void put_seed(long x) {
  char ok = 0;

  if (x > 0)
    x = x % MODULUS; /* correct if x is too large  */
  if (x == 0)
    while (!ok) {
      printf("\nEnter a positive integer seed (9 digits or less) >> ");
      scanf("%ld", &x);
      ok = (0 < x) && (x < MODULUS);
      if (!ok)
        printf("\nInput out of range ... try again\n");
    }
  seed[stream] = x;
}

/*
 * Use this function to get the state of the current random number
 * generator stream.
 */
void get_seed(long *x) { *x = seed[stream]; }

/*
 * Use this function to set the current random number generator
 * stream -- that stream from which the next random number will come.
 */
void select_stream(int index) {
  stream = ((unsigned int)index) % STREAMS;
  if ((initialized == 0) && (stream != 0)) /* protect against        */
    plant_seeds(DEFAULT);                  /* un-initialized streams */
}
