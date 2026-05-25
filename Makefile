PKG_CONFIG ?= pkg-config
DESTDIR    ?= $(EXEC_PREFIX.bash)
# $(*.%) variables come from %.make, generated and included below

override , := ,

override CFLAGS.deps   =
override CPPFLAGS.deps =
override LDFLAGS.deps  =
override LIBS.deps     =
define DRIVE.c
$(CC) $(CFLAGS.deps) -std=c2y $(CFLAGS) \$ 
	  $(CPPFLAGS.deps) -Iinclude -D_GNU_SOURCE $(CPPFLAGS) \$ 
	  $(TARGET_ARCH)
endef
COMPILE.c = $(DRIVE.c) -c
LINK.c    = $(DRIVE.c) $(LDFLAGS.deps) $(LDFLAGS)

ALL_PLUGINS := scriptlocal seek fsync pipe mkmemfd mktempat lnat capset
ALL_DEPS    := bash libcap

.PHONY: all clean install
all: $(addprefix plugins/,$(ALL_PLUGINS))
clean:
	rm -vf $(ALL_DEPS:=.make)
	rm -rf include/bash
	rm -rvf *.o plugins
install: all
	install -Dt $(DESTDIR)/lib/bash plugins/*

%.make: %.make.sh
	PKG_CONFIG=$(PKG_CONFIG) ./$<
include $(ALL_DEPS:=.make)

plugins/%: %.o
	@mkdir -p $(@D)
	$(LINK.c) $+ $(LIBS.deps) $(LDLIBS) -o $@
plugins/scriptlocal: unused.o
$(addprefix plugins/,$(ALL_PLUGINS)): CFLAGS.deps   += $(CFLAGS.bash)
$(addprefix plugins/,$(ALL_PLUGINS)): CPPFLAGS.deps += $(CPPFLAGS.bash)
$(addprefix plugins/,$(ALL_PLUGINS)): LDFLAGS.deps  += $(LDFLAGS.bash)
$(addprefix plugins/,$(ALL_PLUGINS)): LIBS.deps     += $(LIBS.bash)
plugins/capset:                       CFLAGS.deps   += $(CFLAGS.libcap)
plugins/capset:                       CPPFLAGS.deps += $(CPPFLAGS.libcap)
plugins/capset:                       LDFLAGS.deps  += $(LDFLAGS.libcap)
plugins/capset:                       LIBS.deps     += $(LIBS.libcap)

# bash's include files are set up stupidly and the .pc file insists on infecting
# us with it (by -Iing every include/bash/**/ directory)
# Reject this by patching the headers
bash_headers := $(patsubst ./%,%,$(patsubst $(includedir.bash)/bash/%,%,$\
  $(foreach sub,. builtins include,$\
    $(wildcard $(includedir.bash)/bash/$(sub)/*.h))))
  $(patsubst %.c,%.o,$(wildcard *.c)) \
: $(addprefix include/bash/,$(bash_headers)) $(wildcard *.h)

include/bash:
	mkdir -p include
	rm -rf include/bash
	cp -r --no-preserve=all $(includedir.bash)/bash include
	touch include/bash
bash-headers.sed:
	echo '/^#include/{' >$@
	  for header in $(bash_headers) \
	; do printf '  s|["<]%s[">]|<bash/%s>|\n' "$$(basename -- "$$header")" "$$header" >>$@ \
	; done
	echo '}' >>$@
$(addprefix include/bash/,$(bash_headers)) &: include/bash bash-headers.sed
	find include/bash -name '*.h' -exec sed -f bash-headers.sed -i \{} +
	find include/bash -name '*.h' -exec touch -h -- \{} +
.INTERMEDIATE: bash-headers.sed
