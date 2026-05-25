#pragma once

#include <ctype.h>
#include <stdlib.h>

#ifdef __STDC_DEFER_TS25755__
#include <stddefer.h>
#endif

#define STRICT1(M, x) M(x)

#ifndef __STDC_DEFER_TS25755__
struct __defer_anchor { };
#define defer STRICT1(_DEFER, __COUNTER__)
#define _DEFER(U) \
  auto inline void __deferred_ ## U [[gnu::always_inline]]( \
    struct __defer_anchor*); \
  struct __defer_anchor __defer_anchor_ ## U \
    [[gnu::cleanup(__deferred_ ## U)]]; \
  auto inline void __deferred_ ## U [[gnu::always_inline]]( \
    struct __defer_anchor*)
#endif

static inline int strtofd(char const *str) {
  if(!isdigit(*str)) return -1;
  unsigned long parsed = strtoul(str, (char**)&str, 10);
  int fd = parsed;
  if(*str || fd < 0 || fd != parsed) return -1;
  return fd;
}
