#include <errno.h>

#include <bash/builtins.h>
#include <bash/shell.h>
#include <bash/builtins/common.h>

// TODO: It's tempting, from an implementation perspective, to unconditionally
//       print the new offset on stdout (removing the need to meddle with shell
//       variables ourselves). However, to a user who doesn't want the new
//       offset, this might be extremely annoying (presumably the output would
//       need to be suppressed by >/dev/null).
//
//       Figure out what the best interface is.
static int seek_builtin(WORD_LIST *list) {
  bool end = false;
  if(!list) goto usage;
  if(!strcmp("-e", list->word->word)) {
    end = true;
    list = list->next;
  }

  intmax_t fd;
  if(!list) {
    builtin_error("missing fd");
    goto usage;
  } else if(
       !all_digits(list->word->word)
    || !valid_number(list->word->word, &fd)
    || fd < 0 || (int)fd != fd) {
    builtin_error("bad fd: %s", list->word->word);
    goto usage;
  }
  list = list->next;

  bool relative = false;
  off64_t offset = 0;
  if(list) {
    char sign = *list->word->word;
    relative = sign == '+' || sign == '-';
    intmax_t parsed;
    if(  !valid_number(list->word->word, &parsed)
      || (offset = parsed) != parsed) {
      builtin_error("bad offset: %s", list->word->word);
      goto usage;
    }
    list = list->next;
  }

  SHELL_VAR *out = NULL;
  if(list) {
    int vflags, bindflags;
#ifdef ARRAY_VARS
    SET_VFLAGS(list->word->flags, vflags, bindflags);
#else
    vflags = 0;
    bindflags = 0;
#endif
    if( !valid_identifier(list->word->word)
#ifdef ARRAY_VARS
      && !valid_array_reference(list->word->word, vflags)
#endif
    ) {
      sh_invalidid(list->word->word);
      return EXECUTION_FAILURE;
    }
    if(  !(out = builtin_bind_variable(list->word->word, NULL, bindflags))
      || ASSIGN_DISALLOWED(out, 0)) return EX_MISCERROR;
    list = list->next;
  }

  if(list) {
    builtin_error("too many arguments");
    goto usage;
  }

  int whence = relative ? SEEK_CUR : SEEK_SET;
  if(end) whence = SEEK_END;
  offset = lseek64(fd, offset, whence);

  if(offset == -1) {
    builtin_error("seek failed: %s", strerror(errno));
    return EXECUTION_FAILURE;
  }
  if(out) bind_variable_value(out, itos(offset), 0);
  return EXECUTION_SUCCESS;

usage:
  builtin_usage();
  return EX_USAGE;
}

static char const *const seek_doc[] = {
  "Reposition an open file description, like lseek(2).",
  "",
  "Adjust the offset of the open file description referred to by the file",
  "descriptor FD. If OFFSET is specified (without [+|-] and) without -e,",
  "reposition the file description to OFFSET bytes after the start of the",
  "file (i.e. set the position to OFFSET). If {+|-}OFFSET is specified",
  "without -e, reposition the file description the specified signed number",
  "of bytes relative to its current position. If -e is specified, then",
  "reposition the file description to [+|-]OFFSET bytes relative to the end",
  "of the file.",
  "",
  "If [+|-]OFFSET is not specified, it defaults to 0. If VARNAME is",
  "specified, it is set to the new value of the file offset, in bytes",
  "relative to the start of the file.",
  "",
  "Examples:",
  "seek \"$fd\"            \"rewind\" to the start of the file",
  "seek -e \"$fd\"         skip to the end of the file",
  "seek \"$fd\" +0         do nothing",
  "seek \"$fd\" +0 offset  set the shell variable offset to the current",
  """                      position in the file",
  NULL
};

struct builtin seek_struct = {
  "seek",
  seek_builtin,
  BUILTIN_ENABLED,
  (char* const*)seek_doc,
  "seek [-e] {fd} [[+|-]{offset}] [varname]",
  0
};
