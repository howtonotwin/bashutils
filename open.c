// Copyright 2026 Rasheeq Azad <rasheeqazad@howtonotwin.net>
// SPDX-LicenseIdentifier: GPL-3.0-or-later
#include <errno.h>
#include <fcntl.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>
#include <bash/builtins/bashgetopt.h>

#include "util.h"

static mode_t strtomode(char const *str) {
  if(!isdigit(*str)) return -1;
  unsigned long parsed = strtoul(str, (char**)&str, 8);
  mode_t mode = parsed;
  if(*str || mode & ~0o7777 || mode != parsed) return -1;
  return mode;
}

static int open_builtin(WORD_LIST *list) {
  int        dirfd     = AT_FDCWD;
  char const *filepath = ".";
  mode_t     mode;
  bool       read      = false,    write    = false,
             creat     = false,    tmpfile  = false,
             directory = false,    excl     = false,
             nofollow  = false,    nonblock = false,
             noctty    = false,    explicit = false;
  reset_internal_getopt();
getopts:
  while(true) switch(internal_getopt(list, "u:rwc:t:DeLNY")) {
  case 'u':
    dirfd = strtofd(list_optarg);
    if(dirfd == -1) {
      builtin_error("-u: bad file descriptor");
      return EX_USAGE;
    }
    break;
  case 'r':
    read = true;
    break;
  case 'w':
    write = true;
    break;
  case 'c':
    creat = true;
    mode = strtomode(list_optarg);
    if(mode == -1) {
      builtin_error("-c: bad mode");
      return EX_USAGE;
    }
    break;
  case 't':
    tmpfile = true;
    mode = strtomode(list_optarg);
    if(mode == -1) {
      builtin_error("-t: bad mode");
      return EX_USAGE;
    }
    break;
  case 'D':
    directory = true;
    break;
  case 'e':
    excl = true;
    break;
  case 'L':
    nofollow = true;
    break;
  case 'N':
    nonblock = true;
    break;
  case 'Y':
    noctty = true;
    break;
  case -1:
    break getopts;
  default:
    goto usage;
  }
  list = loptend;

  if(list) {
    explicit = true;
    filepath = list->word->word;
    list     = list->next;
  }
  if(list) goto usage;

  int expected_usage_error = 0;
  if(creat && tmpfile) {
    builtin_error("-c and -t specified together");
    // bail instead of letting the syscall fail because we got two modes and
    // already lost one of them
    return EX_USAGE;
  }
  if((creat || tmpfile) && directory) {
    builtin_error("warning: -c/-t and -D are incompatible");
    expected_usage_error = EINVAL;
  }
  if(tmpfile && !write) {
    builtin_error("warning: -t requires -w");
    expected_usage_error = EINVAL;
  }
  if(directory && write) {
    builtin_error("warning: -w and -D are incompatible");
    expected_usage_error = EINVAL;
  }
  if(creat && !explicit) {
    builtin_error("warning: -c requires FILEPATH");
    if(!expected_usage_error) expected_usage_error = EISDIR;
  }

  int flags = 0;
       if(!read && !write) flags |= O_PATH;
  else if(!read &&  write) flags |= O_WRONLY;
  else if( read && !write) flags |= O_RDONLY;
  else if( read &&  write) flags |= O_RDWR;
  if(creat)     flags |= O_CREAT;
  if(tmpfile)   flags |= O_TMPFILE;
  if(directory) flags |= O_DIRECTORY;
  if(excl)      flags |= O_EXCL;
  if(nofollow)  flags |= O_NOFOLLOW;
  if(nonblock)  flags |= O_NONBLOCK;
  if(noctty)    flags |= O_NOCTTY;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized" // ???
  int fd =
      creat || tmpfile
    ? openat(dirfd, filepath, flags, mode)
    : openat(dirfd, filepath, flags);
#pragma GCC diagnostic pop
  if(fd == -1) {
    int error = errno;
    builtin_error("%s", strerror(errno));
    return error == expected_usage_error ? EX_USAGE : EXECUTION_FAILURE;
  }

  if(expected_usage_error) {
    builtin_error(
      "bug?: expected at least %s, but got no error at all"
    , strerrorname_np(expected_usage_error));
  }
  printf("%d\n", fd);
  return sh_chkwrite(EXECUTION_SUCCESS);

usage:
  builtin_usage();
  return EX_USAGE;
}

static char const *const open_doc[] = {
  "Open (and perhaps create) a file, like open(2)/openat(2).",
  "",
  "Open a file, and print out (as a decimal string) a new file descriptor",
  "that the shell process can use to access and refer to the file. In the",
  "base mode of operation (without -t and without -L), the file is found",
  "according to the path FILEPATH and the file descriptor FD (which, if",
  "specified, is expected to be a decimal string). If FILEPATH is relative",
  "and FD has not been specified, FILEPATH is resolved starting from the",
  "current directory. If FD has been specified (with -u), it must refer to",
  "a directory, and FILEPATH is interpreted starting from that directory.",
  "If FILEPATH is not specified, it defaults to . (dot, the name under",
  "which each directory contains an entry to itself). If FILEPATH is",
  "absolute, it is looked up relative to the current root directory and FD",
  "is ignored.",
  "",
  "The mode of the new file descriptor is determined by -r and -w. -r alone",
  "gives read permission (O_RDONLY), -w alone gives write permission",
  "(O_WRONLY), and -rw together give both (O_RDWR). If neither is",
  "specified, that is valid too (O_PATH), in which case the returned file",
  "descriptor is still useful for unambiguously and race condition-immunely",
  "referring to the specified file in the future. Note that the open mode",
  "does not affect the file mode, though the open mode is sometimes subject",
  "to being checked against the file mode.",
  "",
  "There are no flags to this command that correspond to O_APPEND, O_ASYNC,",
  "O_CLOEXEC, O_DIRECT, O_DSYNC, O_NOATIME, or O_SYNC. Most of these can be",
  "applied after the fact by the fdflags builtin.",
  "",
  "The -c (O_CREAT) and -t (O_TMPFILE) flags request the creation of a new",
  "regular file. Thus, they require that the MODE of the new file be",
  "specified. (MODE should be an octal string in the range 0000-7777. The",
  "octal prefix 0 is optional.) MODE will be subject to the current umask.",
  "",
  "For -c (O_CREAT), FILEPATH should be specified (the default is useless)",
  "and must not end in a slash, and all of its components except the last",
  "must exist. If the file specified by FILEPATH does not exist, it is",
  "created. If it does already exist, it is opened (as if -c and MODE were",
  "not specified), but only if it is not a directory and -e was not",
  "specified. If -e (O_EXCL) is specified, the operation fails when it",
  "finds the file to already exist.",
  "",
  "For -t (O_TMPFILE), FILEPATH (together with FD) should instead specify a",
  "directory. A new regular file is created (according to MODE and the",
  "umask) \"in\" that directory, but it is not given a name and does not",
  "appear as an entry in the directory. The file begins life already",
  "\"deleted\". The new file nevertheless belongs to (and is backed by the",
  "storage of) the filesystem of the specified directory and can be read,",
  "written, and otherwise accessed normally. If -e is not specified, the",
  "file can later be linked into a directory of that filesystem using",
  "linkat(2) (or the lnat builtin). If -e (O_EXCL) is specified, the new",
  "file is instead opened with O_EXCL, which prevents it from ever being",
  "linked to a name. Unless the file is eventually linked in to a",
  "directory, it will be deleted when all open file descriptors referring",
  "to it are closed.",
  "",
  "The -D (O_DIRECTORY) flag causes the open to fail if the specified file",
  "is not a directory. The -N (O_NONBLOCK/O_NDELAY) flag has two",
  "independent effects: it causes O_NONBLOCK to be set on the resulting",
  "file descriptor (which can always be set or reset post-open by fdflags)",
  "and it prevents the open operation from blocking in certain conditions",
  "where it normally would. Such effects include stopping open from",
  "blocking on a named pipe, causing open to fail fast if it happens to",
  "break a file lease, and stopping open from waiting for and spinning up",
  "the medium when opening an optical disk drive device file(!). The",
  "-Y (O_NOCTTY) flag prevents the opened file from becoming the",
  "controlling terminal of the shell process (if it is a terminal and the",
  "process doesn't already have one). The -L (O_NOFOLLOW) flag prevents a",
  "symbolic link at the final component of FILEPATH from being followed. If",
  "neither -r nor -w are specified (O_PATH), the open may succeed",
  "(depending on the other flags) and return a file descriptor referring to",
  "the link itself. Otherwise, the open will fail. (Symbolic links in the",
  "non-final components of FILEPATH are always followed.)",
  "",
  "When -t is specified, -w must also be specified. (The presence or",
  "absence of -r is not constrained.) -e may have unexpected effects if it",
  "is used without -c or -t, and thus usually should not be specified",
  "without one of them. In the special case where the opened file is a",
  "block device, -e is meaningful without -c or -t and causes the operation",
  "to fail if the block device is in use by the system (e.g. mounted).",
  "",
  "It is very likely not useful to use this command in a subshell command",
  "substitution:",
  "",
  "\ttmp=$(open -rwt 444)  # tmp is only open in the subshell",
  "\techo words >&$tmp     # will fail",
  "",
  "You will probably want a braced substitution:",
  "",
  "\ttmp=${ open -rwt 444; }",
  "\techo words >&$tmp",
  "\tseek $tmp",
  "\tcat <&$tmp               # output: words",
  NULL
};

struct builtin open_struct = {
  "open",
  open_builtin,
  BUILTIN_ENABLED,
  (char* const*)open_doc,
  "\n\topen [-u {fd}] [-r] [-w] [{-c|-t} {mode}|-D] [-e]"
  "\n\t     [-L] [-N] [-Y] [--] [filepath]",
  0
};
