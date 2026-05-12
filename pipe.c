#include <errno.h>
#include <unistd.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>

static int pipe_builtin(WORD_LIST *list) {
  if(list) goto usage;

  int ends[2];
  if(pipe(ends)) {
    builtin_error("cannot create pipe: %s", strerror(errno));
    return EXECUTION_FAILURE;
  }

  printf("%d %d\n", ends[0], ends[1]);
  return sh_chkwrite(EXECUTION_SUCCESS);

usage:
  builtin_usage();
  return EX_USAGE;
}

static char const *const pipe_doc[] = {
  "Create a pipe, just like pipe(2).",
  "",
  "Create a pipe, as by pipe(2), and print out one file descriptor each",
  "(as decimal strings) for the read and write ends, in that order. The",
  "new file descriptors are opened only in the shell process that calls",
  "pipe, so it is very likely not useful to use this command in a",
  "subshell command subsitution:",
  "",
  "\tpipe=($(pipe))          # pipe[@] are only open in the subshell",
  "\techo words >&${pipe[1]} # will fail",
  "",
  "You will probably want a braced substitution:",
  "",
  "\tpipe=(${ pipe; })",
  "\techo words >&${pipe[1]}",
  NULL
};

struct builtin pipe_struct = {
  "pipe",
  pipe_builtin,
  BUILTIN_ENABLED,
  (char* const*)pipe_doc,
  "pipe {no arguments}",
  0
};
