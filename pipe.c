#include <errno.h>
#include <unistd.h>

#include <bash/config.h>
#include <bash/shell.h>
#include <bash/builtins.h>
#include <bash/builtins/common.h>

#if !ARRAY_VARS
#error "Bash builds without array variables are not supported"
#endif

#define ARRLEN(arr) (__extension__ _Generic(typeof(arr), \
  typeof(*(arr))[sizeof(arr) / sizeof(*(arr))]: sizeof(arr) / sizeof(*(arr))))

int pipe_builtin(WORD_LIST *list) {
  struct {
    bool is_array;
    union {
      SHELL_VAR *array;
      SHELL_VAR *ends[2];
    };
  } destination;

  if(!list) goto usage;
  else if(!list->next) {
    destination.is_array = true;
    // 1: clear the existing array | 2: check for a valid identifier
    if(!(destination.array = builtin_find_indexed_array(list->word->word, 3))) {
      return EXECUTION_FAILURE;
    }
  } else if(!list->next->next) {
    destination.is_array = false;
    WORD_DESC *rwnames[ARRLEN(destination.ends)] = {
      list->word, list->next->word};
    for(size_t i = 0; i < ARRLEN(rwnames); i++) {
      int vflags, bindflags;
      WORD_DESC *rwref = rwnames[i];
      char *rwname = rwref->word;
      SET_VFLAGS(rwref->flags, vflags, bindflags);

      SHELL_VAR **slot = destination.ends + i;

      if(!valid_identifier(rwname) && !valid_array_reference(rwname, vflags)) {
        sh_invalidid(rwname);
        return EXECUTION_FAILURE;
      }
      if(  !(*slot = builtin_bind_variable(rwname, NULL, bindflags))
        || ASSIGN_DISALLOWED(*slot, 0)) return EX_MISCERROR;
    }
  } else goto usage;

  int ends[2];
  if(pipe(ends)) {
    builtin_error("cannot create pipe: %s", strerror(errno));
    return EXECUTION_FAILURE;
  }
  for(size_t i = 0; i < ARRLEN(ends); i++) {
    char *end = itos(ends[i]);
    if(destination.is_array) bind_array_element(destination.array, i, end, 0);
    else bind_variable_value(destination.ends[i], end, 0);
  }
  return EXECUTION_SUCCESS;

usage:
  builtin_usage();
  return EX_USAGE;
}

char const *const pipe_doc[] = {
  "Create a pipe, just like pipe(2).",
  "",
  "Create a pipe, as by pipe(2), and return one file descriptor each for",
  "the read and write ends. If RNAME and WNAME are specified, they are the",
  "names of variables (or array elements) to which the file descriptors",
  "will be stored. If ARRNAME is specified, ARRNAME will be set to the",
  "two-element array (readfd writefd).",
  NULL
};

struct builtin pipe_struct = {
  "pipe",                 // builtin name
  pipe_builtin,           // function implementing the builtin
  BUILTIN_ENABLED,        // initial flags for builtin
  (char* const*)pipe_doc, // array of long documentation strings.
  "pipe {RNAME WNAME | ARRNAME}",
  0
};
