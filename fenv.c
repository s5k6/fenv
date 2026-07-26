#define _GNU_SOURCE 1

#include <stdio.h>
#include <unistd.h>
#include <err.h>
#include <string.h>
#include <stdlib.h>
#include <fnmatch.h>
#include <regex.h>
#include <stdbool.h>



/* Patterns can be matched against the NAME, the VALUE, or the entire
NAME=VALUE string representing an enviroment entry.*/

enum what { NAME, VALUE, WHOLE };


/* Patterns can be interpreted as fixed stringm glob, basic or
extended regular expression.  Perl sucks, as a matter of principle. */

enum algo { FIXED, GLOB, BASIC, EXTENDED };


/* A filter is a pattern, along with metadata on how to use it. */

struct filter {
    bool keep;       // Keep matching entries if true, drop them otherwise.
    enum what what;  // cf. `enum what`
    enum algo algo;  // cf. `enum algo`
    bool fold;
    char *pattern;
    regex_t *regex;
};


/* Verbositu is controlled by the following bits. */

enum verb {
    VPATTERN = 1,    // print patterns
    VMATCH = 2,      // report matching entries and pattern
    VUNMATCHED = 4,  // report unmatched entries
    VVERDICT = 8     // report verdict, keep or drop
};



/* Most options specify how the subsequent patterns are to be used
(glob/regex, case sensitivity). Warn, if such an option goes unused.
E.g., in `fenv -i -I` the `-I` cancels the prior `-i`, making its
presence pointless. */

void chkUnusedOpt(char *unused, char c) {
    if (*unused) {
        if (c)
            errx(1, "Option -%c undoes option -%c, which is not used by "
                    "any pattern.", c, *unused);
        else
            errx(1, "Option -%c is not used by any pattern.", *unused);
    }
    *unused = c;
}



/* Compile regular expression.  Cf. [1] for documentation. */

regex_t *regex(char const *pat, bool fold, bool ext) {

    regex_t *re =malloc(sizeof(regex_t));
    if (!re)
        err(1, "malloc");

    int flags = REG_NOSUB | (fold ? REG_ICASE : 0) | (ext ? REG_EXTENDED : 0);

    int ec = regcomp(re, pat, flags);
    if (ec) {
        size_t len = regerror(ec, re, NULL, 0);
        char *buf = malloc(len);
        if (!buf)
            err(1, "malloc");
        regerror(ec, re, buf, len);
        errx(1, "regex '%s': %s", pat, buf);
    }

    return re;
}



/* Match a fixed string. */

bool matchFix(char const *pat, char const *str, bool fold) {

    if (fold)
        return strcasecmp(pat, str) == 0;

    return strcmp(pat, str) == 0;
}



/* Match a glob. */

bool matchGlob(char const *pat, char const *str, bool fold) {

    int ec = fnmatch(pat, str, fold ? FNM_CASEFOLD : 0);

    switch (ec) {
        case 0:
            return true;
        case FNM_NOMATCH:
            return false;
        default:
            errx(1, "fnmatch error %d", ec);
    }
}



/* Match a compiled regular expression. */

bool matchRegex(regex_t const *re, char const *str) {

    int ec = regexec(re, str, 0, NULL, 0);

    switch (ec) {
        case 0:
            return true;
        case REG_NOMATCH:
            return false;
        case REG_ESPACE:
            errx(1, "regexec ran out of memory");
        default:
            errx(1, "regexec unknown error %d", ec);
    }
}



/* It's pretty straight forward, we do most of this in one go. */

int main(int argc, char **argv) {


    /* Print help if called without arguments. */

    if (argc < 2) {
        printf(
            "\n"
#include "help.str"
            "\n"
        );
        return 0;
    }


    // Iterate over the command line arguments
    // ---------------------------------------

    /* Assume as many filters as there are command line options.
    There cannot be more. */

    struct filter *filters = calloc((size_t)argc, sizeof(struct filter));
    if (!filters)
        err(1, "malloc");
    int filterTop = 0;  // utilisation of filters
#define FILTER (filters[filterTop])

    long int verbosity = 0;  // verbosity setting
    char **cmd = NULL;  // command to run

    /* Current setting of filter modifiers. */
    bool curKeep = false;
    bool curFold = false;
    enum what curWhat = NAME;
    enum algo curAlgo = GLOB;


    /* For each of the four categories of filter modifying options,
    keep track of what options have been given, and whether they were
    used.  A null character indicates that no option was used since
    the last pattern. */

    char unusedKeep = '\0', unusedFold = '\0', unusedWhat = '\0',
        unusedAlgo = '\0';

    for (int i = 1; i < argc; i++) {

        if (argv[i][0] == '-' && argv[i][1] != '-') {

            /* Everything starting with `-` but not `--` is considered
            a string of options.  Iterate over the individual
            characters. */

            for (char *c = argv[i] + 1; *c; c++) {

                /* Check whether an option undoes a prior option that
                remained unused. */
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

                /* Handles all options and detects invalid ones. */
                switch (*c) {

                    case 'k': curKeep = true; break;
                    case 'd': curKeep = false; break;

                    case 'n': curWhat = NAME; break;
                    case 'v': curWhat = VALUE; break;
                    case 'w': curWhat = WHOLE; break;

                    case 'F': curAlgo = FIXED; break;
                    case 'G': curAlgo = GLOB; break;
                    case 'B': curAlgo = BASIC; break;
                    case 'E': curAlgo = EXTENDED; break;

                    case 'i': curFold = true; break;
                    case 'I': curFold = false; break;

                    /* Set verbosity, either by value, or increase by
                    one.  This is a bit ugly, as it does not match the
                    other option parsing logic. */
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

            /* Standalone option `--` indicates "end of options", the
            remaining arguments constitute the command to run. */

            if (i >= argc - 1)
                errx(1, "No command given after `--`.");
            cmd = argv + i + 1;
            break;  // end of options

        } else {

            /* This argument is a pattern.  Mark all modifier
            categories as used. */

            unusedKeep = '\0';
            unusedWhat = '\0';
            unusedAlgo = '\0';
            unusedFold = '\0';

            /* A leading dash `-` in a pattern must be escaped by
            doubling it, i.e., command line argument `--foo`
            represents pattern `-foo`. */
            char *p = argv[i];
            if (strncmp(p, "--", 2) ==0)
                p+=1;

            /* Populate filter with current settings */
            FILTER.keep = curKeep;
            FILTER.what = curWhat;
            FILTER.algo = curAlgo;
            FILTER.fold = curFold;
            FILTER.pattern = p;

            /* For basic and extended regular expressions, compile the
            regex data structure to be used in matching. */
            switch (curAlgo) {

                case FIXED:
                case GLOB:
                    FILTER.regex = NULL;
                    break;

                case BASIC:
                case EXTENDED:
                    FILTER.regex = regex(p, curFold, curAlgo == EXTENDED);
                    break;
            }

            /* Advance the filter counter. */
            filterTop++;
        }
    }

    /* Check, whether an option went unused by any pattern. */
    chkUnusedOpt(&unusedKeep, 0);
    chkUnusedOpt(&unusedWhat, 0);
    chkUnusedOpt(&unusedAlgo, 0);
    chkUnusedOpt(&unusedFold, 0);

    if (!cmd)
        warnx("No command given.");

    if (verbosity > 0)
        warnx("Verbosity is %ld.", verbosity);


    // Print accumulated filters
    // -------------------------

    if (filterTop < 1) {
        warnx("No filters defined, this is pointless.");
    } else if (verbosity & VPATTERN) {
        for (int i = 0; i < filterTop; i++) {
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


    // Construct new environment
    // -------------------------

    /* Determine length of original environment, and allocate buffer
    for new environment.  The new environment has at most as many
    entries as the old one. */

    if (!environ)
        errx(1, "Original environment is NULL");
    size_t environCount = 0;
    while (environ[environCount])
        environCount++;

    char **newenv = calloc(environCount + 1, sizeof(char *));
    if (!newenv)
        err(1, "malloc");
    size_t newenvTop = 0;


    /* A buffer is needed to maintain a copy of an environment
    variable's name. */
    char *name = NULL;
    size_t nameLen = 0;


    /* Iterate over old environment, produce new environment */

    for (int i = 0; environ[i]; i++) {

        // Find value, copy name (so we have it as separate string)
        char *val = strchr(environ[i], '=');
        if (!val) {
            warnx("Dropping pathological entry: %s", environ[i]);
            continue;
        }

        // Determine length of name, and copy to buf, terminating with `\0`.
        size_t l = (size_t)(val - environ[i]);
        if (nameLen <= l) {
            nameLen = 2 * (l + 1);
            name = realloc(name, nameLen);
            if (!name)
                err(1, "malloc");
        }
        memcpy(name, environ[i], l);
        name[l] = '\0';

        val++; // Previously pointing to the `=`, move to start of value.

        /* `curKeep` is what was used for the last pattern specified
        on the command line.  The opposite is used for unmatched
        environment variables, i.e., it's the default now. */
        bool keep = !curKeep;

        /* Iterate over patterns, first match exits the loop. */
        for (int j = 0; j < filterTop; j++) {
            struct filter const f = filters[j];
            bool matched = false;

            /* What to apply the pattern to */
            char *what;
            switch (f.what) {
                case NAME: what = name; break;
                case VALUE: what = val; break;
                case WHOLE: what = environ[i]; break;
            }

            /* Choose matching algorithm */
            switch (f.algo) {

                case FIXED:
                    matched = matchFix(f.pattern, what, f.fold);
                    break;

                case GLOB:
                    matched = matchGlob(f.pattern, what, f.fold);
                    break;

                case BASIC:
                case EXTENDED:
                    matched = matchRegex(f.regex, what);
                    break;
            }

            if (matched) {
                if (verbosity & VMATCH)
                    warnx("Match %d: %s", j, environ[i]);
                keep = f.keep;
                goto matched;  // break out of two nested loops
            }
        }
        if (verbosity & VUNMATCHED)
            warnx("Unmatched: %s", environ[i]);
        matched:

        if (keep)
            newenv[newenvTop++] = environ[i];

        if (verbosity & VVERDICT)
            warnx("%s: %s", keep ? "Keep" : "Drop", environ[i]);
    }


    /* Terminate environment with null pointer.  Trim size */

    newenv[newenvTop++] = NULL;
    newenv = realloc(newenv, newenvTop * sizeof(char*));
    if (!newenv)
        err(1, "malloc");


    // Run the command
    // ---------------

    if (cmd) {
        execvpe(cmd[0], cmd, newenv);
        err(1, "Failed to execute: %s", cmd[0]);
    }

    return 0;
}


/* Notes:
[1]: https://ftp.gnu.org/old-gnu/Manuals/glibc-2.2.5/html_node/Regular-Expressions.html#Regular%20Expressions
*/
