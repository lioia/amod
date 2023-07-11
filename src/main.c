#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_help_screen();
int generate_command(int argc, char **argv);
int run_command(int argc, char **argv);

/*
 *  Usage:
 *    ./amod: show help screen
 *    ./amod --help: show help screen
 *    ./amod generate file: generate 100 instances with
 * different hard-coded parameters
 *    ./amod run workers file: run instances from  with x
 * threads
 *
 *    instances.csv: instance_number,release_date,processing_time
 * */
int main(int argc, char **argv) {
  if (argc == 1) {
    print_help_screen();
    return 0;
  }

  if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-H")) {
    print_help_screen();
    return 0;
  } else if (!strcmp(argv[1], "generate")) {
    return generate_command(argc, argv);
  } else if (!strcmp(argv[1], "run")) {
    return run_command(argc, argv);
  }
}

int generate_command(int argc, char **argv) {
  char *filename = "instances.csv";
  if (argc == 3) {
    filename = argv[2];
  }
  printf("Generating instances in %s\n", filename);
  // TODO: implement
  return 0;
}

int run_command(int argc, char **argv) {
  int workers = 8;
  char *filename = "instances.csv";
  if (argc == 3) {
    workers = atoi(argv[2]);
  } else if (argc == 4) {
    workers = atoi(argv[2]);
    if (workers < 1)
      workers = 1;
    filename = argv[3];
  }
  printf("Running simulation with %d threads on instances from %s\n", workers,
         filename);
  // TODO: implement
  return 0;
}

void print_help_screen() {
  printf("AMOD Project\n\n");
  printf("Usage: amod [COMMAND]\n\n");
  printf("Commands:\n");
  printf("\thelp\tShow help screen\n");
  printf("\tgenerate [filename]\tGenerate instances in filename (default: "
         "instances.csv)\n");
  printf("\trun [workers] [filename]\t\tRun simulation (default: workers: 8, "
         "filename: instances.csv)\n");
}
