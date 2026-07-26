Run command with modified environment.

    fenv
    fenv [OPTIONS...] -- COMMAND...

The first form prints this help.  The second form replaces itself with
COMMAND... after filtering the environment variables.

Patterns.  Each entry from the surrounding environment is matched
against the patterns in the order given on the command line.  For each
entry, only the first matching pattern decides its fate (keep or
drop).

  PATTERN
  --PATTERN
        Words not starting with a dash `-` are considered patterns.
        If the first character of a pattern is desired to be a dash,
        then the leading dash must be doubled

Defaults: Drop entries with names matching a cases-sensitive glob.
This is equivalent to `-dIGn`.

Effect of a match.  Entries matching drop-patterns (the default) are
dropped, those matching keep-patterns are kept.

  -d    The following patterns are drop-patterns.

  -k    The following patterns are keep-patterns.

Unmatched entries.  If the last pattern is a drop-pattern, then
unmatched entries are kept in the taget environment.  Conversely, if
the last pattern is keep-pattern, unmatched entries are dropped.

What to match.  A pattern can be appied to the NAME, the VALUE, or the
entire NAME=VALUE string of an entry in the enviroment table.  These
options only affect the patterns that follow on the command line.

  -n    The following patterns are matched against the name of an
        environment variable.  This is the default, but may be useful
        following `-v` or `-w`.

  -v    The following patterns are matched against the value of an
        environment variable.

  -w    The following patterns are matched against the whole NAME=VALUE
        string representing an environment variable and its value.
        This by convention should contain a `=` character.

How to match.  These options only affect the patterns that follow on
the command line.

  -F    Fixed strings.  The pattern must match the entire string.  Use
        globs or regular expressions to test for substrings.  For
        details, cf. strcmp(3), strcasecmp(3).

  -G    Globs.  This is the default.  The pattern must match the
        entire string.  Use `*PATTERN*` to look for substrings.  For
        details, cf. glob(7), fnmatch(3).

  -B    Basic regular expressions.  The pattern matches on substrings.
        Use anchoring to further constrain matches.  For details,
        cf. regcomp(3).

  -E    Extended regular expressions.  The pattern matches on
        substrings.  Use anchoring to further constrain matches.
        For details, cf. regcomp(3) flag `REG_EXTENDED`.

Case sensitivity.

  -I    Patterns are case-sensitive.  This is the default.

  -i    Ignore case when matching the following patterns.  For
        details, cf. strcasecmp(3), fnmatch(3) flag `FNM_CASEFOLD`, or
        regcomp(3) flag `REG_ICASE`.

The command (and its arguments) to be executed in the modified
environment must be given last on the command line, following the
ubiquitous end-of-options indicator `--`.

  --    End of options.  The following arguments specify the command to
        run, and its arguments.

Debugging.  Various debug information can be printed to stderr.

  -Vn   If n is a decimal integer, then verbosity is set to n,
        otherwise verbosity is increased by one.  Verbosity is
        interpreted as a bitmask, with the following values:

        1   enumerate the filters constructed from the command line
        2   report filter and environment entry on match
        4   report unmatched environment entries
        8   report verdict for each environment entry


Examples.  All the following examples run the command `env`, which
prints its environment.

 1. Drop all entries whose name starts with `GTK_` or `MC_`.  Using
   two glob patterns:

        $ fenv 'GTK_*' 'MC_*' -- env

    Using one extended regular expression (note anchoring):

        $ fenv -E '^(GTK|MC)_.*' -- env

    Using one of each:

        $ fenv -E '^GTK_.*' -G 'MC_*' -- env

 2. Keep only the entries named `HOME` and `USER`:

        $ fenv -k HOME USER -- env

 3. Drop all entries whose name starts with `MC_`, except for
    `MC_XDG_OPEN`.  In the first version, the keep-pattern is
    interpreted as glob, in the second as fixed string:

        $ fenv    -k MC_XDG_OPEN -d    'MC_*' -- env
        $ fenv -F -k MC_XDG_OPEN -d -G 'MC_*' -- env

    Same as the first one above, but the drop pattern is an extended
    regular expression, and the option flags are arranged differently.

        $ fenv -k MC_XDG_OPEN -Ed '^MC_.*' -- env

 4. Drop all variables with an empty *value*:

        $ fenv -v '' -- env

    Drop all entries whose value contains the user's name.  First
    version uses globbing for the pattern, second a regular
    expression.

        $ fenv -v "*${USER}*" -- env
        $ fenv -Ev "${USER}" -- env

 5. Drop the PATH variable if it contains the word `foo`.

        $ fenv -wG "PATH=*foo*" -- env

 6. This may look a tad contrived — keep only entries whose name
    contains an `X`, except for those whose value is a number:

        $ fenv -vE '^[[:digit:]]*$' -knG '*X*' -- env
