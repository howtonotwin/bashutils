#include <assert.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>

#define TMP_STR(str) (((struct { char x[sizeof(str)]; }) { .x = str }).x)
#define TMP_WORD(str) (&(WORD_DESC) { .word = TMP_STR(str) })
#define TMP_WCONS(str, list) (&(WORD_LIST) { \
  .word = TMP_WORD(str), \
  .next = list \
})

static bool at_top() {
  return shell_variables == global_variables;
}

static bool loaded = false;
int scriptlocal_builtin_load(char *builtin [[maybe_unused]]) {
  assert(!strcmp(builtin, "scriptlocal"));
  if(!at_top()) {
    builtin_error("scriptlocal: can only be loaded from top scope");
    return false;
  }
  if(!loaded) global_variables = push_scope(
    0
  , copy_vartab(global_variables->table));
  return loaded = true;
}
static int scriptlocal_builtin(WORD_LIST *list) {
  if(!at_top()) {
    builtin_error("can only be used at top scope");
    return EXECUTION_FAILURE;
  }
  if(!list) {
    // it'd be nice to not special-case this, but set_or_show_attributes (aka
    // declare) will print out all (exported) variables if we don't bail here
    builtin_error("no variables specified");
    return EX_USAGE;
  }
  return set_or_show_attributes(
    TMP_WCONS("-n", TMP_WCONS("--", list))
  , att_exported
  , 0);
}

static char const *const scriptlocal_doc[] = {
  "Make variables local to the current script.",
  "",
  "Make each VARNAME local to the current script, so that changes to them",
  "no longer affect the environment that will be seen by external commands.",
  "Each VALUE is assigned to its corresponding VARNAME, if present."
  "",
  "This can only be done from directly within the top-level scope of a",
  "shell. In other positions, consider `local +x'. (In fact, once",
  "scriptlocal is loaded, it is equivalent to top-level `declare +x'!)",
  "",
  "Repeatedly loading and unloading scriptlocal should probably be avoided.",
  NULL
};

struct builtin scriptlocal_struct = {
  "scriptlocal",
  scriptlocal_builtin,
  BUILTIN_ENABLED,
  (char* const*)scriptlocal_doc,
  "scriptlocal varname[=value] [varname[=value] ...]",
  0
};
