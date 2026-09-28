// Copyright 2026 Rasheeq Azad <rasheeqazad@howtonotwin.net>
// SPDX-LicenseIdentifier: GPL-3.0-or-later
#include <errno.h>
#include <fcntl.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>
#include <bash/builtins/bashgetopt.h>

#include "util.h"

static int sync_open(char const *path) {
  // fsync(2) wants more than O_PATH.
  // Either O_RDONLY or O_WRONLY will do, but read and write permissions are
  // independent. We just have to try twice. Do O_RDONLY second as that's the
  // one we'll report about if it fails.
  // We also need O_NONBLOCK in case the target file is special in some way.
  constexpr int common = O_NONBLOCK | O_CLOEXEC;
  if(int fd = openat(AT_FDCWD, path, O_WRONLY | common); fd != -1) return fd;
  return      openat(AT_FDCWD, path, O_RDONLY | common);
}
static int fsync_builtin(WORD_LIST *list) {
  bool just_data = false, whole_filesystem = false, use_fd = false;
getopts:
  while(true) switch(internal_getopt(list, "dfu")) {
  case 'd':
    just_data = true;
    break;
  case 'f':
    whole_filesystem = true;
    break;
  case 'u':
    use_fd = true;
    break;
  case -1:
    break getopts;
  default:
    goto usage;
  }
  list = loptend;

  if(just_data + whole_filesystem > 1) {
    builtin_error("-d and -f are incompatible");
    return EX_USAGE;
  };
  int (*sync_variant)(int) =
      just_data        ? fdatasync
    : whole_filesystem ? syncfs
    :                    fsync;

  if(!list) {
    builtin_error("missing %s", use_fd ? "FD" : "FILEPATH");
    goto usage;
  }
  char const *target = list->word->word;
  list = list->next;

  if(list) goto usage;

  {
    int fd;
    if(use_fd) {
      fd = strtofd(target);
      if(fd == -1) {
        builtin_error("bad fd: %s", target);
        goto usage;
      }
    } else {
      fd = sync_open(target);
      if(fd == -1) {
        builtin_error("cannot open %s: %s", target, strerror(errno));
        return EXECUTION_FAILURE;
      }
    }
    defer {
      if(!use_fd && close(fd)) {
        builtin_error("closing %s: %s", target, strerror(errno));
      }
    }

    if(sync_variant(fd)) {
      builtin_error("%s", strerror(errno));
      return EXECUTION_FAILURE;
    }
    return EXECUTION_SUCCESS;
  }

usage:
  builtin_usage();
  return EX_USAGE;
}

static char const *const fsync_doc[] = {
  "Commit filesystem changes to disk, like fsync(2) and friends.",
  "",
  "\"Flush\" the cached writes for a single file or filesystem to the",
  "appropriate backing storage. The successful return of this operation",
  "forms a barrier in time, where all reads that occur after the barrier—",
  "even reads that occur after a system crash—will be able to see all",
  "writes that occurred before the barrier.",
  "",
  "The file to operate on may be specified either by giving FILEPATH or by",
  "using -u and giving (as a decimal string) a file descriptor FD referring",
  "to the file. A whole filesystem may be specified as the operand by",
  "specifying -f and giving an FD or FILEPATH referring to a file on that",
  "filesystem. If -d is specified, the flush (and the write barrier)",
  "applies only to the file contents, and not necessarily to any metadata",
  "that is not necessary to recover the contents. Note that -d and -f are",
  "incompatible.",
  NULL
};

struct builtin fsync_struct = {
  "fsync",
  fsync_builtin,
  BUILTIN_ENABLED,
  (char* const*)fsync_doc,
  "fsync [-d|-f] [-u] [--] {fd|filepath}",
  0
};
