vpcc = gcc -std=c99 -g -Wall -Wextra -Wpedantic -Wbad-function-cast \
        -Wconversion -Wwrite-strings -Wstrict-prototypes -Wshadow \
	-Wno-overlength-strings

.PHONY : all test clean distclean

all : fenv

clean :
	rm -rf fenv.help testcases/

distclean : clean
	rm -f fenv

test : fenv
	./testsuite

fenv : fenv.c fenv.help

%.help : %.txt
	sed -E 's/"/\\"/g; s/^/"/; s/$$/\\n"/' $< > $@

% : %.c
	$(vpcc) -o $@ $<
	strip $@
