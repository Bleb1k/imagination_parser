#define NOB_IMPLEMENTATION
#include "deps/nob.h"
#define FLAG_IMPLEMENTATION
#include "deps/flag.h"

Nob_Cmd cmd = {0};

void usage(FILE *stream) {
  fprintf(stream, "Usage: ./main [OPTION] [-- <PASS OPTIONS>] \n");
  fprintf(stream, "OPTIONS:\n");
  flag_print_options(stream);
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF(argc, argv);

  if (!nob_mkdir_if_not_exists("build/"))
    return 1;

  nob_cmd_append(&cmd, "clang", "-Wall", "-Wextra", "-O3", "-o", "build/main",
                 "src/main.c", "-I", "deps/");
  if (!nob_cmd_run_sync_and_reset(&cmd))
    return 1;

  bool *help =
      flag_bool("help", false, "Print this help to stdout and exit with 0");
  bool *run = flag_bool("run", false, "Run program");

  if (!flag_parse(argc, argv)) {
    usage(stderr);
    flag_print_error(stderr);
    exit(1);
  }

  if (*help) {
    usage(stdout);
    exit(0);
  }

  int rest_argc = flag_rest_argc();
  char **rest_argv = flag_rest_argv();

  if (*help) {
    usage(stdout);
    exit(0);
  }

  if (*run) {
    nob_cmd_append(&cmd, "build/main");
    if (rest_argc > 0)
      nob_da_append_many(&cmd, rest_argv, rest_argc);
    if (!nob_cmd_run_sync_and_reset(&cmd))
      return 1;
  }

  return 0;
}
