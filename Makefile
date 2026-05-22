PKG_CONFIG ?= pkg-config
DESTDIR    ?= $(PREFIX.bash)
# $(%.bash) variables come from bash.make, generated and included below

define DRIVE.bash
$(CC) $(CFLAGS.bash) -std=c2y $(CFLAGS) \$ 
	  $(CPPFLAGS.bash) -Iinclude -D_GNU_SOURCE $(CPPFLAGS) \$ 
	  $(TARGET_ARCH)
endef
COMPILE.bash = $(DRIVE.bash) -c
LINK.bash    = $(DRIVE.bash) $(LDFLAGS.bash) $(LDFLAGS)

ALL_PLUGINS := scriptlocal seek fsync pipe mkmemfd mktempat lnat

.PHONY: all clean install
all: $(addprefix plugins/,$(ALL_PLUGINS))
clean:
	rm -rf include/bash bash.make
	rm -rvf *.o plugins
install: all
	install -Dt $(DESTDIR)/lib/bash plugins/*

bash.make: bash.make.sh
	PKG_CONFIG=$(PKG_CONFIG) ./$<
	sed 's/-soname,$$@/-soname,$$(@F)/' -i $@
include bash.make

plugins/%: %.o
	@mkdir -p $(@D)
	$(LINK.bash) $+ $(LIBS.bash) $(LDLIBS) -o $@
%.o: %.c
	$(COMPILE.bash) $< -o $@
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
