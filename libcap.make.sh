#!/bin/sh
# Copyright 2026 Rasheeq Azad <rasheeqazad@howtonotwin.net>
# SPDX-LicenseIdentifier: GPL-3.0-or-later
cat >libcap.make <<EOF
CFLAGS.libcap   = `$PKG_CONFIG libcap --cflags-only-other`
CPPFLAGS.libcap = `$PKG_CONFIG libcap --cflags-only-I`
LDFLAGS.libcap  = `$PKG_CONFIG libcap --libs-only-other --libs-only-L`
LIBS.libcap     = `$PKG_CONFIG libcap --libs-only-l`
EOF
