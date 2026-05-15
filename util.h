#pragma once

#include <ctype.h>
#include <stdlib.h>

static int strtofd(char const *str) {
  if(!isdigit(*str)) return -1;
  unsigned long parsed = strtoul(str, (char**)&str, 10);
  int fd = parsed;
  if(fd < 0 || fd != parsed) return -1;
  return fd;
}
