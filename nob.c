#define NOB_IMPLEMENTATION
#include "deps/nob.h"

#define NOB_WARN_DEPRECATED
#define FLAG_IMPLEMENTATION
#include "deps/flag.h"

Nob_Cmd cmd = {0};

void usage(FILE *stream) {
  fprintf(stream, "Usage: ./main [OPTION] [-- <PASS OPTIONS>] \n");
  fprintf(stream, "OPTIONS:\n");
  flag_print_options(stream);
}

int main(int argc, char **argv) {
  NOB_GO_REBUILD_URSELF_PLUS(argc, argv, "deps/nob.h", "deps/flag.h");

  bool *help =
      flag_bool("help", false, "Print this help to stdout and exit with 0");
  bool *run = flag_bool("run", false, "Run program");
  bool *dbg = flag_bool("dbg", false, "Debug program");
  bool *valgrind = flag_bool("valgrind", false, "Run wrapped in valgrind");

  if (!flag_parse(argc, argv)) {
    usage(stderr);
    flag_print_error(stderr);
    exit(1);
  }

  int rest_argc = flag_rest_argc();
  char **rest_argv = flag_rest_argv();

  if (*help) {
    usage(stdout);
    exit(0);
  }

  if (!mkdir_if_not_exists("build/"))
    return 1;

  cmd_append(&cmd, "clang", "-std=c23", "-Wall", "-Wextra");
  if (*valgrind || *dbg)
    cmd_append(&cmd, "-mno-avx", "-mno-avx2", "-mno-fma", "-O0", "-ggdb3");
  cmd_append(&cmd, "-o", "build/main", "src/main.c", "-I", "deps/");
  if (!cmd_run(&cmd))
    return 1;

  if (*run) {
    if (*valgrind)
      cmd_append(&cmd, "valgrind", "--leak-check=full",
                 "--show-leak-kinds=all");
    if (*dbg) {
      if (*valgrind)
        nob_log(
            WARNING,
            "can't debug with valgrind running, valgrind will take priority");
      else {
        cmd_append(&cmd, "gf2", "--args");
      }
    }
    cmd_append(&cmd, "build/main");
    if (rest_argc > 0)
      da_append_many(&cmd, rest_argv, rest_argc);

    if (!cmd_run(&cmd))
      return 1;
  }

  return 0;
}
