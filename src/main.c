#include "generate/generate.h"
#include "run/run.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int print_help_screen();
int generate_command(int argc, char **argv);
int run_command(int argc, char **argv);

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
  char *folder = "output";
  char *filename = "instances.csv";
  if (argc == 4) {
    folder = argv[3];
    filename = argv[3];
  }
  int result = generate(folder, filename);
  if (result)
    perror("Error while generating instances");
  return result;
}

int run_command(int argc, char **argv) {
  int workers = 8;
  char *filename = "output/instances.csv";
  if (argc == 3)
    filename = argv[3];

  printf("Running simulation with on instances from %s\n", filename);
  return run(filename);
}

int print_help_screen() {
  printf("AMOD Project\n\n");
  printf("Usage:\n");
  printf(
      "\tamod [filename]\t\t\tRun simulation (default: output/instances.csv\n");
  printf("\tamod help\t\t\tShow help screen\n");
  printf("\tamod generate [folder filename]\tGenerate instances in filename "
         "(default: output instances.csv)\n");
  return 0;
}
