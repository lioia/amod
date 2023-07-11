#pragma once

#define NUMBER_OF_INSTANCES 4
// Stream 0
#define NUMBER_OF_JOBS_UL                                                      \
  (int[]) { 10, 25, 50 }
// Stream 1
#define PROCESSING_TIMES_UL                                                    \
  (int[]) { 5, 10, 15 }
// Stream 2
#define RELEASE_DATES_UL                                                       \
  (int[]) { 5, 15, 25 }

int generate(const char *filename);

// from rngs in Discrete Event Simulation, Leemis Park
#define MODULUS 2147483647 /* DON'T CHANGE THIS VALUE                  */
#define MULTIPLIER 48271   /* DON'T CHANGE THIS VALUE                  */
#define CHECK 399268537    /* DON'T CHANGE THIS VALUE                  */
#define STREAMS 256        /* # of streams, DON'T CHANGE THIS VALUE    */
#define A256 22925         /* jump multiplier, DON'T CHANGE THIS VALUE */
#define DEFAULT 123456789  /* initial seed, use 0 < DEFAULT < MODULUS  */

static long seed[STREAMS] = {DEFAULT}; /* current state of each stream   */
static int stream = 0;                 /* stream index, 0 is the default */
static int initialized = 0;            /* test for stream initialization */
int uniform(int a, int b);
double rng(void);
void plant_seeds(long x);
void get_seed(long *x);
void put_seed(long x);
void select_stream(int index);
