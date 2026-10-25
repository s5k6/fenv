
DESTDIR ?=
PREFIX ?= $(DESTDIR)/usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man/

vpcc = gcc -std=c99 -g -Wall -Wextra -Wpedantic -Wbad-function-cast \
  -Wconversion -Wwrite-strings -Wstrict-prototypes -Wshadow \
  -Wno-overlength-strings


.PHONY : all test clean distclean install

all : fenv fenv.1.gz

test : all
	tests/run

clean :
	rm -f license.inc manual.inc short.inc topics.inc version.inc \
	  version.txt

distclean : clean
	rm -f fenv fenv.1.gz

install : fenv fenv.1.gz
	install -d "$(BINDIR)"
	install -t "$(BINDIR)" -m 755 fenv
	install -d "$(MANDIR)/man1"
	install -t "$(MANDIR)/man1" -m 644 fenv.1.gz


fenv : fenv.c short.inc manual.inc license.inc topics.inc version.inc

% : %.c
	$(vpcc) -o $@ $<
	strip $@

*.inc : txt2inc

license.inc : LICENSE
	./txt2inc $< >| $@

version.txt :
	git describe --dirty --match='version_*' > version.txt

version.inc : version.txt
	sed -i 's/^version_//' version.txt
	./txt2inc -n version.txt >| $@

manual.inc : manual.md
	./txt2inc $< >| $@

%.inc : %.txt
	./txt2inc $< >| $@

fenv.1.gz : manual.md
	pandoc \
	  -V title='FENV' -V section=1 \
	  --from markdown --to man --standalone -o fenv.1 $<
	rm -f $@
	gzip fenv.1
