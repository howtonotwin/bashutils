#!/bin/sh
cat >bash.make <<EOF
PREFIX.bash     = `$PKG_CONFIG bash --variable=prefix`
CFLAGS.bash     = `$PKG_CONFIG bash --variable=SHOBJ_CFLAGS`
CPPFLAGS.bash   = `$PKG_CONFIG bash --variable=CCFLAGS`
LDFLAGS.bash    = `$PKG_CONFIG bash --variable=SHOBJ_LDFLAGS`
LIBS.bash       = `$PKG_CONFIG bash --variable=SHOBJ_LIBS`
includedir.bash = `$PKG_CONFIG bash --variable=includedir`
EOF
