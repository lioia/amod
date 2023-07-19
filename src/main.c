#include "generate/generate.h"
#include "run/run.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int print_help_screen();
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
  if (argc > 1) {
    if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-H"))
      return print_help_screen();
    else if (!strcmp(argv[1], "generate"))
      return generate_command(argc, argv);
  }
  return run_command(argc, argv);
}

int generate_command(int argc, char **argv) {
  char *filename = "instances.csv";
  if (argc == 3) {
    filename = argv[2];
  }
  printf("Generating instances in %s\n", filename);
  int result = generate(filename);
  if (result)
    perror("Error while generating instances");
  return result;
}

int run_command(int argc, char **argv) {
  int workers = 8;
  char *filename = "instances.csv";
  if (argc == 3)
    filename = argv[3];

  printf("Running simulation with on instances from %s\n", filename);
  return run(filename);
}

int print_help_screen() {
  printf("AMOD Project\n\n");
  printf("Usage: amod [COMMAND]\n\n");
  printf("Commands:\n");
  printf("\thelp\t\t\t\tShow help screen\n");
  printf("\tgenerate [filename]\t\tGenerate instances in filename (default: "
         "instances.csv)\n");
  printf("\trun [workers] [filename]\tRun simulation (default: workers: 8, "
         "filename: instances.csv)\n");
  return 0;
}
