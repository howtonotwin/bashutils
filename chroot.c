#include <errno.h>
#include <unistd.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>

static int chroot_builtin(WORD_LIST *list) {
  if(list && !strcmp(list->word->word, "--")) list = list->next;
  if(!list || list->next) {
    builtin_usage();
    return EX_USAGE;
  }

  if(chroot(list->word->word)) {
    builtin_error("%s", strerror(errno));
    return EXECUTION_FAILURE;
  }
  return EXECUTION_SUCCESS;
}

static char const *const chroot_doc[] = {
  "Set the current root directory, just like chroot(2).",
  "",
  "Given a path NEWROOT leading to some directory, set that directory as",
  "the current root directory of the shell process. Note that the current",
  "working directory is not changed—invoke cd / if/when you want to.",
  NULL
};

struct builtin chroot_struct = {
  "chroot",
  chroot_builtin,
  BUILTIN_ENABLED,
  (char* const*)chroot_doc,
  "chroot [--] {newroot}",
  0
};
