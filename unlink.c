// Copyright 2026 Rasheeq Azad <rasheeqazad@howtonotwin.net>
// SPDX-LicenseIdentifier: GPL-3.0-or-later
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>
#include <bash/builtins/bashgetopt.h>

#include "util.h"

static int unlink_builtin(WORD_LIST *list) {
  int dirfd = AT_FDCWD;
  reset_internal_getopt();
getopts:
  while(true) switch(internal_getopt(list, "u:")) {
  case 'u':
    dirfd = strtofd(list_optarg);
    if(dirfd == -1) {
      builtin_error("-u: bad file descriptor");
      return EX_USAGE;
    }
    break;
  case -1:
    break getopts;
  default:
    goto usage;
  }
  list = loptend;

  if(!list || list->next) goto usage;

  if(unlinkat(dirfd, list->word->word, 0)) {
    builtin_error("%s", strerror(errno));
    return EXECUTION_FAILURE;
  }
  return EXECUTION_SUCCESS;

usage:
  builtin_usage();
  return EX_USAGE;
}

static char const *const unlink_doc[] = {
  "Remove a name for a file, just like unlinkat(2).",
  "",
  "Given a path FILEPATH naming some directory entry, delete that entry",
  "from its containing directory. This may result in the file pointed to by",
  "that entry being \"deleted\" too, if it has no other names or users. If",
  "FILEPATH is relative, it is interpreted relative to the directory",
  "referred to by the file descriptor FD, if FD is specified (in which case",
  "it is expected to be a decimal string). If FD is not specified, FILEPATH",
  "is taken relative to the current directory.",
  NULL
};

struct builtin unlink_struct = {
  "unlink",
  unlink_builtin,
  BUILTIN_ENABLED,
  (char* const*)unlink_doc,
  "unlink [-u {fd}] [--] {filepath}",
  0
};
