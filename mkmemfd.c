#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>

static int mkmemfd_builtin(WORD_LIST *list) {
  char const *name;
  if(!list) goto usage;
  name = list->word->word;
  list = list->next;

  if(list) goto usage;

  int memfd = memfd_create(name, 0);
  if(memfd == -1) {
    builtin_error("cannot create memfd: %s", strerror(errno));
    return EXECUTION_FAILURE;
  }

  printf("%d\n", memfd);
  return sh_chkwrite(EXECUTION_SUCCESS);

usage:
  builtin_usage();
  return EX_USAGE;
}

static char const *const mkmemfd_doc[] = {
  "Create a file not on any filesystem, just like memfd_create(2).",
  "",
  "Create an ephemeral file that does not belong to any filesystem in",
  "particular, existing only in memory (or swap), as by memfd_create(2).",
  "On success, this command exits with status 0 and prints out (as a",
  "decimal string) the number of a newly opened file descriptor",
  "referring to the new file. On failure, the exit status is nonzero and",
  "an error message is sent to stderr.",
  "",
  "The returned file descriptor is opened O_RDWR, and is the only way to",
  "refer to the new file. Like any other file that is no longer linked",
  "into a filesystem, the file will be deleted once all open file",
  "descriptions referring to it are closed. Thus, it is very likely not",
  "useful to use this command in a subshell command substitution:",
  "",
  "\tmemfd=$(mkmemfd name) # memfd is only open in the subshell",
  "\techo words >&$memfd   # will fail",
  "",
  "You will probably want a braced substitution:",
  "",
  "\tmemfd=${ mkmemfd name; }",
  "\techo words >&$memfd",
  "\tseek $memfd",
  "\tcat <&$memfd             # output: words",
  "",
  "New open descriptions can be opened for the file by opening its magic",
  "symlink in /proc/self/fd (if procfs is mounted). The (mandatory!)",
  "NAME argument will appear in the target of the link, for informative",
  "purposes. NAME otherwise has no effect. The flags argument of",
  "memfd_create(2) currently cannot be specified, so the new file will",
  "immediately have F_SEAL_SEAL (and no other seals).",
  NULL
};

struct builtin mkmemfd_struct = {
  "mkmemfd",
  mkmemfd_builtin,
  BUILTIN_ENABLED,
  (char* const*)mkmemfd_doc,
  "mkmemfd {name}",
  0
};
