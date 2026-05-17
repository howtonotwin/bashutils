#include <errno.h>
#include <fcntl.h>
#include <stdbit.h>
#include <stdcountof.h>
#include <sys/random.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>
#include <bash/builtins/bashgetopt.h>

#include "util.h"

static    char       mathmod_uc(
  unsigned char dividend, unsigned char divisor) {
  return divisor ? dividend % divisor : dividend;
}
constexpr char       safe_chars[] = {
  'A',      'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M'
, 'N',      'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k',      'm'
, 'n',      'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'
,           '2', '3', '4', '5', '6', '7',      '9'
};
constexpr char       mktemp_suffix[] = ".XXXXXX";
typedef   char       mktemp_key[countof mktemp_suffix - countof "."];
static    mktemp_key *mktemp_template(
  char buffer[static restrict NAME_MAX + 1], char const *restrict name) {
  size_t space_for_name = NAME_MAX + 1 - countof mktemp_suffix;
  if(*name != '.') {
    *buffer++ = '.';
    space_for_name--;
  }

  if(char *post = memccpy(buffer, name, 0, space_for_name)) {
    buffer = post - 1;
  } else buffer += space_for_name;

  *buffer++ = '.';
  mktemp_key *slot = (void*)buffer;
  buffer += sizeof *slot;
  *buffer++ = 0;

  return slot;
}
static    bool       generate_mktemp_key(mktemp_key *slot) {
  char    buffer[256 /* getrandom(2) guaranteed buffer size */];
  ssize_t buffered = 0;
  for(char *out = *slot; out != (void*)(slot + 1);) {
    if(!buffered) {
      buffered = getrandom(buffer, sizeof buffer, 0);
      if(buffered == -1) return true;
      if(buffered != sizeof buffer) {
        errno = ENOMSG;
        return true;
      }
    }
    size_t i = mathmod_uc(
      buffer[countof buffer - buffered--]
    , stdc_bit_ceil_uc(countof safe_chars));
    if(i < countof safe_chars) *out++ = safe_chars[i];
  }
  return false;
}

static int opendirat(int relfd, char const *path) {
  return openat(relfd, *path ? path : ".", O_PATH | O_DIRECTORY | O_CLOEXEC);
}
static void maybe_close(int const *fd) {
  if(*fd != -1) close(*fd);
}
static int overlinkat(
  int srcdirfd, char const *srcnam
, int tgtdirfd, char const *tgtnam) {
  char tmpnam[NAME_MAX + 1];
  auto tmpkey = mktemp_template(tmpnam, tgtnam);
  for(unsigned char attempts = 0; ++attempts;) {
    if(generate_mktemp_key(tmpkey)) {
      builtin_error("-f: failed to generate a temporary name");
      return EXECUTION_FAILURE;
    }
    if(attempts % 5 == 0) {
      builtin_error(
        "-f: on attempt #%10u at creating a temporary entry..."
      , attempts);
    }

    if(!linkat(srcdirfd, srcnam, tgtdirfd, tmpnam, AT_EMPTY_PATH)) errno = 0;
    if(errno != EEXIST) break;
  }
  if(errno == EEXIST) builtin_error("-f: giving up...");
  if(errno) {
    builtin_error("%s", strerror(errno));
    return EXECUTION_FAILURE;
  }

  // at long last...
  int ret;
  if(renameat(tgtdirfd, tmpnam, tgtdirfd, tgtnam)) {
    builtin_error("%s", strerror(errno));
    ret = EXECUTION_FAILURE;
  } else ret = EXECUTION_SUCCESS;

  if(unlinkat(tgtdirfd, tmpnam, 0)) {
    builtin_error("removing temporary entry: %s", strerror(errno));
  }
  return ret;
}

static int lnat_builtin(WORD_LIST *list) {
  bool overwrite = false;
  int srcrel = AT_FDCWD, tgtrel = AT_FDCWD;

  reset_internal_getopt();
getopts:
  while(true) switch(internal_getopt(list, "fS:T:")) {
  case 'f':
    overwrite = true;
    break;
  case 'S':
    srcrel = strtofd(list_optarg);
    if(srcrel == -1) {
      builtin_error("-S: bad file descriptor");
      return EX_USAGE;
    }
    break;
  case 'T':
    tgtrel = strtofd(list_optarg);
    if(tgtrel == -1) {
      builtin_error("-T: bad file descriptor");
      return EX_USAGE;
    }
    break;
  case -1:
    break getopts;
  default:
    goto usage;
  }
  list = loptend;

  if(!list) goto usage;
  char const *srcdir = list->word->word;
  list = list->next;

  if(!list) goto usage;
  char const *srcnam = list->word->word;
  if(!*srcnam) {
    if(*srcdir) {
      builtin_error("if SRCNAM=\"\", SRCDIR should also be empty");
      return EX_USAGE;
    }
    if(srcrel == AT_FDCWD) {
      builtin_error("if SRCNAM=\"\", -S SRCREL should be specified");
      return EX_USAGE;
    }
  }
  if(strchr(srcnam, '/')) {
    builtin_error("SRCNAM should not contain path separators");
    return EX_USAGE;
  }
  list = list->next;

  if(!list) goto usage;
  char const *tgtdir = list->word->word;
  list = list->next;

  char const *tgtnam;
  if(!list) {
    if(!*srcnam) {
      builtin_error("if SRCNAM=\"\", TGTNAM is mandatory");
      return EX_USAGE;
    }
    tgtnam = srcnam;
  } else {
    tgtnam = list->word->word;
    if(!*tgtnam) {
      builtin_error("TGTNAM cannot be empty");
      return EX_USAGE;
    }
    if(strchr(tgtnam, '/')) {
      builtin_error("TGTNAM should not contain path separators");
      return EX_USAGE;
    }
    list = list->next;
  }

  if(list) goto usage;

  {
    int srcdirfd [[gnu::cleanup(maybe_close)]];
    if(!*srcnam) {
      srcdirfd = dup(srcrel);
      if(srcdirfd == -1) {
        builtin_error("cannot get source file descriptor: %s", strerror(errno));
      }
    } else {
      srcdirfd = opendirat(srcrel, srcdir);
      if(srcdirfd == -1) {
        builtin_error("cannot open source directory: %s", strerror(errno));
      }
    }
    if(srcdirfd == -1) return EXECUTION_FAILURE;

    int tgtdirfd [[gnu::cleanup(maybe_close)]] = opendirat(tgtrel, tgtdir);
    if(tgtdirfd == -1) {
      builtin_error("cannot open target directory: %s", strerror(errno));
      return EXECUTION_FAILURE;
    }

    if(!linkat(srcdirfd, srcnam, tgtdirfd, tgtnam, AT_EMPTY_PATH)) {
      return EXECUTION_SUCCESS;
    }
    if(overwrite && errno == EEXIST) {
      return overlinkat(srcdirfd, srcnam, tgtdirfd, tgtnam);
    }

    builtin_error("%s", strerror(errno));
    return EXECUTION_FAILURE;
  }

usage:
  builtin_usage();
  return EX_USAGE;
}

static char const *const lnat_doc[] = {
  "Create a new name for an existing file, like ln(1) or linkat(2).",
  "",
  "In the directory specified by the file descriptor TGTREL and the path",
  "TGTDIR, create an entry linking the name TGTNAM to the same file to",
  "which the entry for the name SRCNAM in the directory specified by the",
  "file descriptor SRCREL and the path SRCDIR also links. If TGTNAM is",
  "omitted, it defaults to SRCNAM. Neither SRCNAM nor TGTNAM may contain",
  "slashes. In the normal mode of operation, SRCNAM and TGTNAM must both be",
  "nonempty strings.",
  "",
  "If SRCDIR is a relative path, it is interpreted relative to SRCREL if",
  "SRCREL has been specified and is otherwise interpreted relative to the",
  "current directory. If SRCDIR is an absolute path, it is interpreted",
  "without reference to SRCREL. If SRCDIR is the empty string, the source",
  "directory is SRCREL (if specified) or the current directory.",
  "",
  "Similarly, if TGTDIR is a relative path, it is interpreted relative to",
  "TGTREL if TGTREL has been specified and is otherwise interpreted",
  "relative to the current directory. If TGTDIR is an absolute path, it is",
  "interpreted without reference to TGTREL. If TGTDIR is the empty string,",
  "the target directory is TGTREL (if specified) or the current directory.",
  "",
  "In a special mode of operation, SRCNAM may be an empty string, in which",
  "case SRCDIR must also be an empty string and SRCREL must be specified.",
  "In this special mode of operation, the entry for TGTNAM in the target",
  "directory will be made to link to the file referred to by SRCREL. This",
  "mode allows linking a file to a name without already knowing another",
  "name for that file. Therefore, the OS may subject this mode of",
  "operation to extra checks. (On Linux: this operation generally requires",
  "the CAP_DAC_READ_SEARCH capability, except in the extra special case",
  "where SRCREL was created by openat(2) with flags O_TMPFILE & ~O_EXCL,",
  "where no special privileges are required.)",
  "",
  "Unless -f is specified, this operation will do nothing and fail if the",
  "target directory already contains an entry for the name TGTNAM. If -f is",
  "specified, an attempt will be made to overwrite any such existing entry.",
  "Without -f, the effect of this operation on the target directory is",
  "atomic. With -f, it is not. During an invocation of this command with -f",
  "(or if such an operation is interrupted, and also if the system crashes",
  "before the new state of the target directory makes it to durable",
  "storage), the target directory may be observed in a state where it",
  "contains both the old TGTNAM entry (or lack thereof) and a temporary",
  "entry (with a randomized name) referring to the source file. However,",
  "the effect on the TGTNAM entry itself is always atomic.",
  // We don't want to use renameat directly on the source entry, because
  // a) we don't want to remove the source,
  // b) we may not be *able* to remove the source, and
  // c) in the SRCNAM="" mode, renameat doesn't support AT_EMPTY_PATH.
  // We must
  // 1. create a temporary entry, and
  // 2. rename the temporary entry into the target entry.
  // We can only rely on write access to the target directory, so that's where
  // we must create the temporary entry, and that's where a failed or incomplete
  // operation will be observable.
  NULL
};

struct builtin lnat_struct = {
  "lnat",
  lnat_builtin,
  BUILTIN_ENABLED,
  (char* const*)lnat_doc,
  "lnat [-f] [-S srcrel] [-T tgtrel] [--] {srcdir} {srcnam} {tgtdir} [tgtnam]",
  0
};
