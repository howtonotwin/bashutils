ifndef PKG_CONFIG
PKG_CONFIG := pkg-config
endif
COMPILE.c = \
  $(CC) -std=c23 -fPIC $(CFLAGS) \
  -Iinclude -D_GNU_SOURCE $(CPPFLAGS) \
  $(TARGET_ARCH) -c

ALL_PLUGINS := pipe scriptlocal seek

.PHONY: all clean install
all: $(addprefix plugins/,$(ALL_PLUGINS))
clean:
	rm -rf include/bash
	rm -rvf *.o plugins
install: all
	install -Dt $(DESTDIR)/lib/bash plugins/*

plugins/%:
	@mkdir -p $(@D)
	$(CC) -shared -Wl,-soname,$(@F) $(LDFLAGS) \
	  $(TARGET_ARCH) $(filter-out %.h %/,$+) $(LDLIBS) \
	  -o $@
plugins/pipe: pipe.o
plugins/scriptlocal: scriptlocal.o unused.o
plugins/seek: seek.o

# bash's include files are set up stupidly and the .pc file insists on infecting
# us with it (by -Iing every include/bash/**/ directory)
# Reject this by patching the headers
bash_includedir := $(shell $(PKG_CONFIG) bash --variable=includedir)
bash_headers    := $(patsubst ./%,%,$(patsubst $(bash_includedir)/bash/%,%,$\
  $(foreach sub,. builtins include,$\
    $(wildcard $(bash_includedir)/bash/$(sub)/*.h))))
$(patsubst %.c,%.o,$(wildcard *.c)): $(addprefix include/bash/,$(bash_headers))

include/bash:
	mkdir -p include
	rm -rf include/bash
	cp -r --no-preserve=all $(bash_includedir)/bash include
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
