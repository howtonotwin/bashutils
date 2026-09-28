// Copyright 1987-2025 Free Software Foundation, Inc.
// Copyright 2026 Rasheeq Azad <rasheeqazad@howtonotwin.net>
// SPDX-LicenseIdentifier: GPL-3.0-or-later
#include <bash/builtins.h>
#include <bash/shell.h>

// bash declares some functions in its headers that it doesn't always define
// we have to provide our own definitions
// if bash DOES define these (if it was built with INCLUDE_UNUSED set), then the
// dynamic linker will prefer bash's definitions

// the following are mostly copied from bash-5.3/variables.c

static void *copy_variable_erased(void *entry_erased) {
  return copy_variable(entry_erased);
}
HASH_TABLE *copy_vartab(HASH_TABLE *table) {
  return table ? hash_copy(table, copy_variable_erased) : nullptr;
}
SHELL_VAR *copy_variable(SHELL_VAR *entry) {
  if(!entry) return nullptr;

  SHELL_VAR *ret = xmalloc(sizeof(*ret));
  *ret = *entry;

  ret->name = savestring(ret->name);
  ret->exportstr = COPY_EXPORTSTR(ret);
  if(function_p(ret)) var_setfunc(ret, copy_command(function_cell(ret)));
#ifdef ARRAY_VARS
  else if(array_p(ret)) var_setarray(ret, array_copy(array_cell(ret)));
  else if(assoc_p(ret)) var_setassoc(ret, assoc_copy(assoc_cell(ret)));
#endif
  else if(value_cell(ret)) var_setvalue(ret, savestring(value_cell(ret)));

  return ret;
}
