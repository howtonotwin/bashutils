COMPILE.c = \
  $(CC) -std=c23 -fPIC $(CFLAGS) \
  -Iinclude -D_GNU_SOURCE $(CPPFLAGS) \
  $(TARGET_ARCH) -c

ALL_PLUGINS := pipe

.PHONY: all clean install
all: $(addprefix plugins/,$(ALL_PLUGINS))
clean:
	rm -rf include/bash
	rm -rvf *.o plugins
install: all
	install -Dt $(DESTDIR)/lib/bash plugins/*

$(patsubst %.c,%.o,$(wildcard *.c)): include/bash
plugins/%:
	@mkdir -p $(@D)
	$(CC) -shared -Wl,-soname,$(@F) $(LDFLAGS) \
	  $(TARGET_ARCH) $(filter-out %.h %/,$+) $(LDLIBS) \
	  -o $@
plugins/pipe: pipe.o

# bash's include files are set up stupidly and the .pc file insists on infecting
# us with it (by -Iing every include/bash/**/ directory)
# Reject this by patching the headers
bash_includedir := $(shell pkg-config bash --variable=includedir)
bash_headers := $(patsubst ./%,%,$(patsubst $(bash_includedir)/bash/%,%,$\
  $(foreach sub,. builtins include,$\
    $(wildcard $(bash_includedir)/bash/$(sub)/*.h))))
  include/bash include/bash/builtins include/bash/include \
	$(addprefix include/bash/,$(bash_headers)) \
&: $(addprefix $(bash_includedir)/bash/,$(bash_headers))
	mkdir -p include
	rm -rf include/bash
	cp -r --no-preserve=all $(bash_includedir)/bash include
	$(let tmp,$(shell mktemp),$\
		$(file >$(tmp),/^#include/{)$\
		$(foreach header,$(bash_headers),$\
			$(file >>$(tmp),  s|["<]$(notdir $(header))[">]|<bash/$(header)>|))$\
		$(file >>$(tmp),})$\
	  find include/bash -name '*.h' -exec sed -f $(tmp) -i \{} +)
