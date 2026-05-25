#!/bin/sh
cat >libcap.make <<EOF
CFLAGS.libcap   = `$PKG_CONFIG libcap --cflags-only-other`
CPPFLAGS.libcap = `$PKG_CONFIG libcap --cflags-only-I`
LDFLAGS.libcap  = `$PKG_CONFIG libcap --libs-only-other --libs-only-L`
LIBS.libcap     = `$PKG_CONFIG libcap --libs-only-l`
EOF
