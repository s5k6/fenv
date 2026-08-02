
Name
====

`fenv` — run a program in a filtered environment.


Synopsis
========

    fenv
    fenv [OPTIONS...] [-- COMMAND...]

The first form prints this help.  The second form replaces itself with
COMMAND... after filtering the environment variables.  Not giving a
command may be useful for debugging with option `-V`.


Description
===========

Each entry in the set of current environment variables is matched
against a list of **patterns** given in OPTIONS.

The OPTIONS are either patterns, or **flags** modifying the semantics
of subsequent patterns (with the exception of `-V`).  The default (no
flags given) is to drop entries whose name matches a cases-sensitive
glob.  This is equivalent to `-dnIG`.

Example: Keep only entries whose name contains an `X`, except for
those whose value is a whole decimal number:

    $ fenv -vE '^[[:digit:]]*$' -knG '*X*' -- env

More useful examples at the end, as this may look a little contrived.


Patterns
--------

The preceding flags determine whether a pattern is used as fixed
string, glob (default), basic or extended regular expression, whether
it is case sensitive (default), and whether it is applied to only the
NAME (default), VALUE, or the entire NAME=VALUE entry.

`PATTERN`, `--PATTERN`
: Command line arguments before `--` not starting with a dash are
  considered patterns.  If the first character of a pattern is desired
  to be a dash, then the leading dash must be doubled.


Flags
-----

The effect of a flag applies to all following patterns, until
overridden by another flag of the same group.

Flags may be grouped together, i.e., `-iBv` is equivalent to `-i -B
-v`, matching case insensive basic regular expressions against values
from the environment.


### Effect of a match

The first matching pattern determines the fate of the entry: Matching
a drop-pattern removes the entry from the environment, matching a
keep-pattern keeps the entry.  In either case, this entry is not
tested against following patterns.

`-d`
: The following patterns are drop-patterns.

`-k`
: The following patterns are keep-patterns.

If the last pattern is a drop-pattern, then unmatched entries are kept
in the environment.  Conversely, if the last pattern is keep-pattern,
unmatched entries are dropped.


### Pattern syntax

`-F`
: The following patterns are fixed string.  The pattern must match the
  entire strings.  For details, cf. strcmp(3), strcasecmp(3).

`-G`
: The following patterns are globs.  This is the default.  The pattern
  must match the entire string.  Use `*PATTERN*` to test for
  substrings.  For details, cf. glob(7), fnmatch(3).

`-B`
: The following pattern are basic regular expressions.  These match on
  substrings.  Use *anchoring* to further constrain matches.  For
  details, cf. regcomp(3).

`-E`
: The following pattern are extended regular expressions.  These
  matches on substrings.  Use anchoring to further constrain matches.
  For details, cf. regcomp(3) flag `REG_EXTENDED`.


### Case sensitivity

`-I`
: The following patterns are case-sensitive.  This is the default.

`-i`
: Ignore case in the following patterns.  For details,
  cf. strcasecmp(3) for fixed strings, the `FNM_CASEFOLD` flag of
  fnmatch(3) for globs, or the `REG_ICASE` flag of regcomp(3) for
  regular expressions.


### What to match

`-n`
: The following patterns are matched against the name of an
  environment variable.  This is the default.

`-v`
: The following patterns are matched against the value of an
  environment variable.

`-w`
: The following patterns are matched against the whole NAME=VALUE
  string representing an environment variable and its value.


### Debugging

This is the only global option.  It does not affect patterns.

`-V`, `-V`N
: If decimal integer N is given, then verbosity is set to N, otherwise
  it is increased by one.  In any case, verbosity is interpreted as a
  bitmask, with the following values:

      1   enumerate the filters constructed from the command line
      2   report filter and environment entry on match
      4   report unmatched environment entries
      8   report verdict for each environment entry


Command
-------

The command (and its arguments) to be executed in the modified
environment must be given last on the command line, following the
ubiquitous end-of-options indicator `--`.

`--`
: End of options.  The following arguments specify the command to run,
  and its arguments.


Examples
========

The following examples run the command `env`, which prints its
environment.

 1. Blacklisting: Drop all entries whose name starts with `GTK_` or
    `MC_`.  Using two glob patterns:

        $ fenv 'GTK_*' 'MC_*' -- env

    Using one extended regular expression (note anchoring):

        $ fenv -E '^(GTK|MC)_.*' -- env

 2. Whitelisting: Keep only the entries named `HOME` and `USER`:

        $ fenv -k HOME USER -- env

 3. Exceptions: Drop all entries whose name starts with `MC_`, except
    for `MC_XDG_OPEN`:

        $ fenv -k MC_XDG_OPEN -d 'MC_*' -- env

 4. Looking at values: Drop all variables with an empty value:

        $ fenv -v '' -- env

    Drop all entries whose value contains the user's name.  First
    version uses globbing for the pattern, second a regular
    expression.

        $ fenv -v "*${USER}*" -- env
        $ fenv -Ev "${USER}" -- env

 5. Match name *and* value: Drop the PATH variable if it contains the
    word `foo`.

        $ fenv -wG "PATH=*foo*" -- env

 6. Pattern with leading dash: Drop all environment variables whose
    value starts with a dash:

        $ fenv -v '--*' -- env
