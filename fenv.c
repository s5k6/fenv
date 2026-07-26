#define _GNU_SOURCE 1

#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <fnmatch.h>
#include <regex.h>
#include <stdbool.h>


enum what { NAME, VALUE, WHOLE };
enum algo { FIXED, GLOB, BASIC, EXTENDED };
enum verb {
    VPATTERN = 1,
    VMATCH = 2,
    VUNMATCHED = 4,
    VVERDICT = 8,
    VCMD = 16
};

struct filter {
    bool keep;
    enum what what;
    enum algo algo;
    bool fold;
    char *pattern;
    regex_t *regex;
};


extern char **environ;



void chkUnusedOpt(char *unused, char c) {
    if (*unused) {
        if (c)
            errx(1, "No pattern following `-%c` before `-%c`", *unused, c);
        else
            errx(1, "No pattern following `-%c` towards the end", *unused);
    }
    *unused = c;
}



int main(int argc, char **argv) {

    if (argc < 2) {
        printf(
            "\n"
#include "help.str"
            "\n"
        );
        return 0;
    }


    // iterate over command line, collect filters

    struct filter *filters = calloc((size_t)argc, sizeof(struct filter));
    if (!filters)
        err(1, "malloc");

    bool kindGiven = false;
    bool curKeep = false;
    enum what curWhat = NAME;
    enum algo curAlgo = GLOB;
    bool curFold = false;
    char unusedKeep = 0, unusedWhat = 0, unusedAlgo = 0, unusedFold = 0;
    int fc = 0;
    long int verbosity = 0;
    char **cmd = NULL;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && argv[i][1] != '-') {

            for (char *c = argv[i] + 1; *c; c++) {

                switch (*c) {

                    case 'k':
                    case 'd':
                        chkUnusedOpt(&unusedKeep, *c);
                        break;

                    case 'n':
                    case 'v':
                    case 'w':
                        chkUnusedOpt(&unusedWhat, *c);
                        break;

                    case 'F':
                    case 'G':
                    case 'B':
                    case 'E':
                        chkUnusedOpt(&unusedAlgo, *c);
                        break;

                    case 'i':
                    case 'I':
                        chkUnusedOpt(&unusedFold, *c);
                        break;
                }

                switch (*c) {

                    case 'k':
                        curKeep = true;
                        kindGiven = true;
                        break;
                    case 'd':
                        curKeep = false;
                        kindGiven = true;
                        break;

                    case 'n': curWhat = NAME; break;
                    case 'v': curWhat = VALUE; break;
                    case 'w': curWhat = WHOLE; break;

                    case 'F': curAlgo = FIXED; break;
                    case 'G': curAlgo = GLOB; break;
                    case 'B': curAlgo = BASIC; break;
                    case 'E': curAlgo = EXTENDED; break;

                    case 'i': curFold = true; break;
                    case 'I': curFold = false; break;

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

            if (!kindGiven)
                errx(1, "Keep (`-k`) or drop (`-d`) must be given "
                        "before the first pattern.");

            unusedKeep = 0;
            unusedWhat = 0;
            unusedAlgo = 0;
            unusedFold = 0;

            char *p = argv[i];
            if (strncmp(p, "--", 2) ==0)
                p+=1;
            filters[fc].keep = curKeep;
            filters[fc].what = curWhat;
            filters[fc].algo = curAlgo;
            filters[fc].fold = curFold;
            filters[fc].pattern = p;

            switch (curAlgo) {

                case FIXED:
                case GLOB:
                    filters[fc].regex = NULL;
                    break;

                case BASIC:
                case EXTENDED:
                    {
                        // Cf. [1] for documentation.
#define RE (filters[fc].regex)
                        RE = malloc(sizeof(regex_t));
                        if (!RE)
                            err(1, "malloc");
                        int ec = regcomp(
                            RE, p,
                            REG_NOSUB | (curFold ? REG_ICASE : 0) |
                                (curAlgo == EXTENDED ? REG_EXTENDED : 0)
                        );
                        if (ec) {
                            size_t l = regerror(ec, RE, NULL, 0);
                            char *b = malloc(l);
                            regerror(ec, RE, b, l);
                            if (!b)
                                err(1, "malloc");
                            errx(1, "regex '%s': %s", p, b);
#undef RE
                        }
                    }
            }

            fc++;
        }
    }
    endOfOptions:

    chkUnusedOpt(&unusedKeep, 0);
    chkUnusedOpt(&unusedWhat, 0);
    chkUnusedOpt(&unusedAlgo, 0);
    chkUnusedOpt(&unusedFold, 0);

    if (!cmd)
        warnx("No command given.");

    if (verbosity > 0)
        warnx("Verbosity is %ld.", verbosity);

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
                case FIXED: a = "fixed"; break;
                case GLOB: a = "glob"; break;
                case BASIC: a = "basic"; break;
                case EXTENDED: a = "extended"; break;
            }
            warnx(
                "Pattern %d: %s %s %s %s: %s",
                i, k, w, a,
                filters[i].fold ? "fold" : "case",
                filters[i].pattern
            );
        }
    }


    // determine length of original environment

    if (!*environ)
        errx(1, "Environment is NULL");
    int oc = 0;
    while (environ[oc]) oc++;


    // iterate over old environment, produce new environment

    char **newenv = calloc((size_t)oc, sizeof(char *));
    if (!newenv)
        err(1, "malloc");
    int nc = 0;

    char *buf = NULL;
    size_t bufLen = 0;

    for (int i = 0; environ[i]; i++) {

        // find value, copy name (so we have it as separate string)
        char *val = strchr(environ[i], '=');
        if (!val) {
            warnx("Dropping pathological entry: %s", environ[i]);
            continue;
        }
        size_t nameLen = (size_t)(val - environ[i]);
        val++; // was previously pointing to the `=`
        if (bufLen <= nameLen) {
            bufLen = 2 * (nameLen + 1);
            buf = realloc(buf, bufLen);
            if (!buf)
                err(1, "malloc");
        }
        strncpy(buf, environ[i], nameLen);
        buf[nameLen] = '\0';

        /* `curKeep` is what was used for the last pattern.  The opposite
        is used for unmatched environment variables. */
        bool keep = !curKeep;

        for (int j = 0; j < fc; j++) {
            bool matched = false;

            char *what;
            switch (filters[j].what) {
                case NAME: what = buf; break;
                case VALUE: what = val; break;
                case WHOLE: what = environ[i]; break;
            }

            switch (filters[j].algo) {

                case FIXED:
                    matched = strcmp(filters[j].pattern, what) == 0;
                    break;

                case GLOB:
                    {
                        int ec = fnmatch(
                                      filters[j].pattern,
                                      what,
                                      filters[j].fold ? FNM_CASEFOLD : 0
                                  );
                        switch (ec) {
                            case 0:
                                matched = true;
                                break;
                            case FNM_NOMATCH:
                                matched = false;
                                break;
                            default:
                                errx(1, "fnmatch error");
                        }
                    }
                    break;

                case BASIC:
                case EXTENDED:
                    {
                        int ec = regexec(filters[j].regex, what, 0, NULL, 0);
                        switch (ec) {
                            case 0:
                                matched = true;
                                break;
                            case REG_NOMATCH:
                                matched = false;
                                break;
                            case REG_ESPACE:
                                errx(1, "regexec ran out of memory");
                                break;
                            default:
                                errx(1, "regexec unknown error %d", ec);
                        }
                    }
                    break;
            }

            if (matched) {
                if (verbosity & VMATCH)
                    warnx("Match %d: %s", j, environ[i]);
                keep = filters[j].keep;
                goto matched;
            }
        }
        if (verbosity & VUNMATCHED)
            warnx("Unmatched: %s", environ[i]);
        matched:

        if (keep)
            newenv[nc++] = environ[i];

        if (verbosity & VVERDICT)
            warnx("%s: %s", keep ? "Keep" : "Drop", environ[i]);
    }
    newenv[nc] = NULL;


    // run the command

    if (cmd) {
        execvpe(cmd[0], cmd, newenv);
        err(1, "Failed to execute: %s", cmd[0]);
    }

    return 0;
}


/* Notes:
[1]: https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.5/html_node/Regular-Expressions.html#Regular%20Expressions
*/
