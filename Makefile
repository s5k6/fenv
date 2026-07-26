vpcc = gcc -std=c99 -g -Wall -Wextra -Wpedantic -Wbad-function-cast \
        -Wconversion -Wwrite-strings -Wstrict-prototypes -Wshadow \
	-Wno-overlength-strings

.PHONY : all test clean distclean

all : fenv

clean :
	rm -rf *.str

distclean : clean
	rm -f fenv

fenv : fenv.c help.str

%.str : %.txt
	sed -E 's/"/\\"/g; s/^/"/; s/$$/\\n"/' $< > $@

% : %.c
	$(vpcc) -o $@ $<
	strip $@
