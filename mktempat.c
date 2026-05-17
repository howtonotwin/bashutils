#include <errno.h>
#include <fcntl.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>
#include <bash/builtins/bashgetopt.h>

#include "util.h"

static int mktempat_builtin(WORD_LIST *list) {
  int  excl_flag = 0;
  bool dir_is_fd = false;

  reset_internal_getopt();
getopts:
  while(true) switch(internal_getopt(list, "eu")) {
  case 'e':
    excl_flag = O_EXCL;
    break;
  case 'u':
    dir_is_fd = true;
    break;
  case -1:
    break getopts;
  default:
    goto usage;
  }
  list = loptend;

  char const *dirarg;
  if(list) {
    dirarg = list->word->word;
    list   = list->next;
  } else dirarg = nullptr;

  if(list) goto usage;

  char const *dirpath = dir_is_fd || !dirarg ? "." : dirarg;
  int dirdirfd;
  if(dir_is_fd && dirarg) {
    dirdirfd = strtofd(dirarg);
    if(dirdirfd == -1) goto usage;
  } else dirdirfd = AT_FDCWD;

  int tmpfile = openat(
    dirdirfd, dirpath, O_TMPFILE | O_RDWR | excl_flag, 0o777);
  if(tmpfile == -1) {
    builtin_error("cannot open tmpfile: %s", strerror(errno));
    return EXECUTION_FAILURE;
  }

  printf("%d\n", tmpfile);
  return sh_chkwrite(EXECUTION_SUCCESS);

usage:
  builtin_usage();
  return EX_USAGE;
}

static char const *const mktempat_doc[] = {
  "Create a file with no name, just like open(2) with O_TMPFILE.",
  "",
  "Create a file with no name, and print out (as a decimal string) a",
  "newly opened file descriptor referring to the file. If DIR is",
  "specified, it determines the directory in which to create the new",
  "file. If DIR is not specified, the current directory is used. If -u",
  "is specified, DIR is expected to be a file descriptor (as a decimal",
  "string) referring to a directory. If -u is not specified, DIR is",
  "expected to be a path to the target directory.",
  "",
  "The new file, though it is not linked into DIR or any other",
  "directory, still belongs to (and is backed by the storage of) the",
  "filesystem of DIR. If -e is not specified, the file can later be",
  "linked into a directory of that filesystem using linkat(2). If -e is",
  "specified, the new file is instead opened with O_EXCL, which prevents",
  "it from ever being linked to a name. Unless the file is eventually",
  "linked in to a directory, it will be deleted when all open file",
  "descriptors referring to it are closed.",
  "",
  "The file is created with mode 0777 (ugo=rwx), subject to the",
  "prevailing umask. The file is always opened O_RDWR.",
  "",
  "It is very likely not useful to use this command in a subshell",
  "command substitution:",
  "",
  "\ttmp=$(mktempat)      # tmp is only open in the subshell",
  "\techo words >&$tmp    # will fail",
  "",
  "You will probably want a braced substitution:",
  "",
  "\ttmp=${ umask 0333; mktempat; }",
  "\techo words >&$tmp",
  "\tseek $tmp",
  "\tcat <&$tmp                     # output: words",
  NULL
};

struct builtin mktempat_struct = {
  "mktempat",
  mktempat_builtin,
  BUILTIN_ENABLED,
  (char* const*)mktempat_doc,
  "mktempat [-e] [-u] [--] [dir]",
  0
};
