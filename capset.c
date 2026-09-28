// Copyright 2026 Rasheeq Azad <rasheeqazad@howtonotwin.net>
// SPDX-LicenseIdentifier: GPL-3.0-or-later
#include <errno.h>
#include <stdckdint.h>
#include <unistd.h>

#include <sys/capability.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>

#include "util.h"

static int capset_builtin(WORD_LIST *list) {
  size_t total_space = 0;
  for(auto it = list; it; it = it->next) {
    if(  ckd_add(&total_space, total_space, strlen(it->word->word))
      || !++total_space) {
      builtin_error("description too long!?");
      return EX_USAGE;
    }
  }

  char buffer[total_space ?: 1], *out = buffer;
  size_t remaining_space = total_space;
  for(auto it = list; it; it = it->next) {
    char *next = memccpy(out, it->word->word, 0, remaining_space);
    if(!next) {
      builtin_error("arguments mysteriously got longer!?");
      return EXECUTION_FAILURE;
    }
    remaining_space -= next - out;
    next[-1] = ' ';
    out = next;
  }
  if(total_space && remaining_space) {
    builtin_error("arguments mysteriously got shorter!?");
    return EXECUTION_FAILURE;
  }
  out[-1] = 0;

  cap_t caps = cap_from_text(buffer);
  if(!caps) {
    int error = errno;
    builtin_error("cap_from_text: %s", strerror(errno));
    return error == EINVAL ? EX_USAGE : EXECUTION_FAILURE;
  }
  defer { if(cap_free(caps)) builtin_error("cap_free: %s", strerror(errno)); }

  if(cap_set_proc(caps)) {
    builtin_error("%s", strerror(errno));
    return EXECUTION_FAILURE;
  }
  return EXECUTION_SUCCESS;
}

static char const *const capset_doc[] = {
  "Manipulate the current process's main capability sets.",
  "",
  "Given a textual description of the desired values for the effective,",
  "inheritable, and permitted capability sets, apply those settings to the",
  "shell process. The textual description is as parsed by cap_from_text(3)",
  "like for e.g. capsh(1). For convenience, if multiple arguments are",
  "passed, they will be concatenated with spaces separating them.",
  NULL
};

struct builtin capset_struct = {
  "capset",
  capset_builtin,
  BUILTIN_ENABLED,
  (char* const*)capset_doc,
  "capset [clause [...]]",
  0
};
