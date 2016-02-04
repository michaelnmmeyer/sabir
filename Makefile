PREFIX = /usr/local

CFLAGS = -DSB_PREFIX='"$(PREFIX)"' -DSB_DEBUG
CFLAGS += -std=c11 -g -Wall -Werror -pedantic
#CFLAGS += -O2 -DNDEBUG -march=native -mtune=native -fomit-frame-pointer -s
#CFLAGS += -flto -fdata-sections -ffunction-sections -Wl,--gc-sections
LDLIBS = -lm

AMALG = sabir.h sabir.c

#--------------------------------------
# Abstract targets
#--------------------------------------

all: $(AMALG) sabir example

clean:
	rm -f sabir example vgcore* core test/*.tmp

check: sabir
	test/test.py test/data/*
	test/bad_utf8.sh

install: sabir sabir-train model.sb
	install -spm 0755 sabir $(PREFIX)/bin/sabir
	install -pm 0644 cmd/sabir.1 $(PREFIX)/share/man/man1
	install -pm 0755 sabir-train $(PREFIX)/bin/sabir-train
	install -pDm 0644 model.sb $(PREFIX)/share/sabir/model.sb

uninstall:
	rm -f $(PREFIX)/bin/sabir
	rm -f $(PREFIX)/share/man/man1/sabir.1
	rm -f $(PREFIX)/bin/sabir-train
	rm -f $(PREFIX)/share/sabir/model.sb
	rmdir $(PREFIX)/share/sabir 2> /dev/null || true

.PHONY: all clean check install uninstall

#--------------------------------------
# Concrete targets
#--------------------------------------

cmd/%.ih: cmd/%.txt
	cmd/mkcstring.py < $< > $@

sabir.h: src/api.h
	cp $< $@

sabir.c: $(wildcard src/*.[hc] src/lib/*.[hc])
	src/mkamalg.py src/*.c > $@

example: example.c $(AMALG)
	$(CC) $(CFLAGS) $(LDLIBS) $< sabir.c src/lib/utf8proc.c -o $@

sabir: $(wildcard cmd/*) $(AMALG)
	$(CC) $(CFLAGS) $(LDLIBS) cmd/*.c sabir.c src/lib/utf8proc.c -o $@
