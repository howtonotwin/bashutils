PKG_CONFIG ?= pkg-config
DESTDIR    ?= $(PREFIX.bash)
# $(%.bash) variables come from bash.make, generated and included below

define DRIVE.c
$(CC) $(CFLAGS.bash) -std=c2y $(CFLAGS) \$ 
	$(CPPFLAGS.bash) -Iinclude -D_GNU_SOURCE $(CPPFLAGS) \$ 
	$(TARGET_ARCH)
endef
COMPILE.c = $(DRIVE.c) -c
LINK.c    = $(DRIVE.c) $(LDFLAGS.bash) $(LDFLAGS)

ALL_PLUGINS := scriptlocal seek fsync pipe mkmemfd mktempat lnat

.PHONY: all clean install
all: $(addprefix plugins/,$(ALL_PLUGINS))
clean:
	rm -rf include/bash
	rm -rvf *.o plugins
install: all
	install -Dt $(DESTDIR)/lib/bash plugins/*

bash.make: bash.make.sh
	./$<
	sed 's/-soname,$$@/-soname,$$(@F)/' -i $@
include bash.make

plugins/%: %.o
	@mkdir -p $(@D)
	$(LINK.c) $+ $(LIBS.bash) $(LDLIBS) -o $@
plugins/scriptlocal: unused.o

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
