#define _GNU_SOURCE 1

#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>

enum bool { FALSE = 0, TRUE = 1 };
enum what { NAME, VALUE, WHOLE };
enum algo { FIXED /* EXTENDED BASIC PERL GLOB */ };
enum verb {
    VPATTERN = 1,
    VMATCH = 2,
    VUNMATCHED = 4,
    VVERDICT = 8,
    VCMD = 16
};

struct filter {
    enum bool keep;
    enum what what;
    enum algo algo;
    char *pattern;    // depending on algo, this may better be a union
};


extern char **environ;


int main(int argc, char **argv) {

    if (argc < 2) {
        printf(
            "\n"
#include "fenv.help"
            "\n"
        );
        return 0;
    }


    // iterate over command line, collect filters

    struct filter *filters = calloc((size_t)argc, sizeof(struct filter));
    if (!filters) err(1, "malloc");

    int kindGiven = -1;
    enum bool keep = FALSE;
    enum what what = NAME;
    enum algo algo = FIXED;
    int fc = 0;
    long int verbosity = 0;
    char **cmd = NULL;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '-') {

            for (char *c = argv[i] + 1; *c; c++) {
                switch (*c) {

                    case 'k':
                        keep = TRUE;
                        kindGiven = i;
                        break;
                    case 'd':
                        keep = FALSE;
                        kindGiven = i;
                        break;

                    case 'n':
                        what = NAME;
                        break;
                    case 'v':
                        errx(1, "Matching values not implemented.");
                        what = VALUE;
                        break;
                    case 'w':
                        errx(1, "Matching whole entries not implemented.");
                        what = WHOLE;
                        break;

                    case 'F':
                        algo = FIXED;
                        break;
                    case 'E':
                    case 'B':
                    case 'P':
                    case 'G':
                        errx(1, "Syntax -%c not implemented.", *c);
                        break;

                    case 'V':
                        {
                            char *q = c + 1;
                            long int v = strtol(q, &c, 10);
                            if (c == q)
                                verbosity++;
                            else
                                verbosity = v;
                            c--;

                            if (verbosity > 15)
                                errx(1, "Verbosity too big.");

                        }
                        break;

                    default:
                        errx(1, "Invalid option: -%c", *c);
                }
            }

        } else if (strcmp(argv[i], "--") == 0) {
            if (i >= argc - 1)
                errx(1, "No command given after `--`.");
            cmd = argv + i + 1;
            goto endOfOptions;

        } else {

            if (kindGiven < 0)
                errx(1, "Keep (`-k`) or drop (`-d`) must be given "
                        "before first pattern.");

            kindGiven = 0;  // used to complain if kind is not used

            char *p = argv[i];
            if (strncmp(p, "--", 2) ==0) p+=1;
            filters[fc].keep = keep;
            filters[fc].what = what;
            filters[fc].algo = algo;
            filters[fc].pattern = p;

            fc++;
        }
    }
    endOfOptions:

    if (kindGiven > 0)
        errx(1, "Keep (`-k`) or drop (`-d`) must be followed by a pattern.");

    if (verbosity > 0)
        warnx("Verbosity is %ld.", verbosity);

    if (!cmd) {
        warnx("No command given, will print filters.");
        verbosity = 1;
    }

    // print accumulated filters

    if (fc < 1) {
        warnx("No filters defined, this is pointless.");
    } else if (verbosity & VPATTERN) {
        for (int i = 0; i < fc; i++) {
            char const *k, *w, *a;
            k = filters[i].keep ? "keep" : "drop";
            switch (filters[i].what) {
                case NAME: w = "name"; break;
                case VALUE: w = "value"; break;
                case WHOLE: w = "whole"; break;
            }
            switch (filters[i].algo) {
                case FIXED: a = "fix"; break;
            }
            warnx("Pattern %d: %s %s %s: %s", i, k, w, a, filters[i].pattern);
        }
    }


    // determine length of original environment

    if (!*environ) errx(1, "Environment is NULL");
    int oc = 0;
    while (environ[oc]) oc++;


    // iterate over old environment, produce new environment

    char **newenv = calloc((size_t)oc, sizeof(char *));
    if (!newenv) err(1, "malloc");
    int nc = 0;

    for (int i = 0; environ[i]; i++) {

        // find length of name and pointer to value
        char *val = strchr(environ[i], '=');
        if (!val) {
            warnx("Pathological entry: %s", environ[i]);
            continue;
        }
        size_t nameLen = (size_t)(val - environ[i]);

        /* `keep` is what was used for the last pattern.  The opposite
        is used for unmatched environment variables. */
        enum bool verdict = !keep;

        for (int j = 0; j < fc; j++) {
            enum bool matched = FALSE;
            switch (filters[j].algo) {

                case FIXED:
                    if (strncmp(filters[j].pattern, environ[i], nameLen) == 0) {
                        matched = TRUE;
                    }
                    break;

            }
            if (matched) {
                if (verbosity & VMATCH) warnx("Match %d: %s", j, environ[i]);
                verdict = filters[j].keep;
                goto matched;
            }
        }
        if (verbosity & VUNMATCHED) warnx("Unmatched: %s", environ[i]);

        matched:

        if (verdict)
            newenv[nc++] = environ[i];

        if (verbosity & VVERDICT)
            warnx("%s: %s", verdict ? "Keep" : "Drop", environ[i]);
    }
    newenv[nc] = NULL;


    // run the command

    if (cmd) {
        execvpe(cmd[0], cmd, newenv);
        err(1, "Failed to execute: %s", cmd[0]);
    }

    return 0;
}
