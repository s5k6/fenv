vpcc = gcc -std=c99 -g -Wall -Wextra -Wpedantic -Wbad-function-cast \
        -Wconversion -Wwrite-strings -Wstrict-prototypes -Wshadow \
	-Wno-overlength-strings

.PHONY : all test clean distclean

all : fenv fenv.1.gz

clean :
	rm -rf fenv *.inc fenv.1.gz version.txt

distclean : clean
	rm -f fenv fenv.1.gz

test : all
	tests/run

fenv : fenv.c short.inc long.inc license.inc topics.inc version.inc

% : %.c
	$(vpcc) -o $@ $<
	strip $@

*.inc : txt2inc

license.inc : LICENSE txt2inc
	./txt2inc $< >| $@

version.txt :
	git describe --dirty --match='version_*' > version.txt

version.inc : version.txt txt2inc
	sed -i 's/^version_//' version.txt
	./txt2inc -n version.txt >| $@

long.inc : long.md txt2inc
	./txt2inc $< >| $@

%.inc : %.txt txt2inc
	./txt2inc $< >| $@

fenv.1.gz : long.md
	pandoc --standalone --from markdown --to man -o fenv.1 $<
	gzip fenv.1
