#!/bin/sh
# Copyright 2026 Rasheeq Azad <rasheeqazad@howtonotwin.net>
# SPDX-LicenseIdentifier: GPL-3.0-or-later
cat >bash.make <<EOF
EXEC_PREFIX.bash = `$PKG_CONFIG bash --variable=exec_prefix`
CFLAGS.bash      = `$PKG_CONFIG bash --variable=SHOBJ_CFLAGS`
CPPFLAGS.bash    = `$PKG_CONFIG bash --variable=CCFLAGS`
LDFLAGS.bash     = ` $PKG_CONFIG bash --variable=SHOBJ_LDFLAGS \
                   | sed 's/-soname,$@/-soname,$(@F)/'`
LIBS.bash        = `$PKG_CONFIG bash --variable=SHOBJ_LIBS`
includedir.bash  = `$PKG_CONFIG bash --variable=includedir`
EOF
