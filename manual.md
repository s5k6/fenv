Name
====

`fenv` — run a program in a filtered environment.


Synopsis
========

    fenv OPTIONS... [-- COMMAND...]

The first form replaces itself with COMMAND after filtering the
environment according to the patterns defined in OPTIONS.

    fenv [-h [TOPIC]]

The second form prints brief usage hints (without arguments), a list
of topics (if only `-h` is present), or more detailed information on a
specified topic.


Description
===========

Each Linux process has an “environment” (cf. environ(7) for details)
which is inherited from parent to child processes.  Typically, env(1)
is used to add or change environment entries before running a command,
or to completely clear the environment.

This `fenv` provides a more fine-grained means to sanitise the
environment by pattern matching on variable names, values, or both.


Invocation
----------

The standalone `--` separates the OPTIONS from the COMMAND and its
arguments to be run in the modified environment.  Not giving a command
may be useful for debugging with option `-V`.

Every OPTION is either a **pattern**, or a combination of **flags**.
A flag affects the semantics of subsequent patterns.

Each entry in the current environment is matched against the patterns,
in the order given on the command line.  The first matching pattern
determines whether the entry is kept (`-k`) or dropped (`-d`).

By default (no flags given) patterns are interpreted as case-sensitive
globs against variable names, and matching entries are dropped.


Patterns
--------

`PATTERN`, `--PATTERN`
: Patterns are distinguished from flags by the absence of a leading
  dash.  If the first character of a pattern is desired to be a dash,
  then the leading dash must be doubled.

The preceding flags determine syntax and effect of a pattern, see
below for details.


Flags
-----

The effect of a flag applies to all following patterns, until
overridden by another flag affecting the same trait.

Flags may be grouped together, e.g., the following combinations are
all equivalent: `-cBv`, `-c -B -v`, `-cB -v` and `-c -Bv`.


### Effect of a match

The first matching pattern determines the fate of the entry: Matching
a drop-pattern removes the entry from the environment, matching a
keep-pattern keeps the entry.  In either case, the matching entry is
not tested against any further pattern.

`-d`
: The following patterns are drop-patterns.

`-k`
: The following patterns are keep-patterns.

If the last pattern is a drop-pattern, then unmatched entries are kept
in the environment.  Conversely, if the last pattern is a
keep-pattern, then unmatched entries are dropped.


### Pattern syntax

`-F`
: The following patterns are fixed strings.  A pattern must match the
  entire string.  For details, cf. strcmp(3), strcasecmp(3).

`-G`
: The following patterns are globs.  This is the default.  A pattern
  must match the entire string.  Use `*PATTERN*` to test for
  substrings.  For details, cf. glob(7), fnmatch(3).

`-B`
: The following patterns are basic regular expressions.  These match
  on substrings.  Use *anchoring* to further constrain matches.  For
  details, cf. regex(7), regcomp(3).

`-E`
: The following patterns are extended regular expressions.  These
  matches on substrings.  Use *anchoring* to further constrain
  matches.  For details, cf. regex(7), and flag `REG_EXTENDED` of
  regcomp(3).


### Case sensitivity

`-C`
: The following patterns are case-sensitive.  This is the default.

`-c`
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

Verbosity is controled by a bitmask.  Some bits are per filter, i.e,
setting them affects only the patterns specified later on the command
line.  The other bits are per environment entry, the last setting used
on the command line is used.

`-V`, `-V`N
: Set the bitmask to N if present, or *all* if absent.

The following bits apply to all subsequent patterns:

    1   show filter and its ID before processing starts
    2   report filter ID and envrironment entry on match
    4   report filter ID and envrironment entry on mismatch

The last setting of the following bits applies to environment entries:

    8   report unmatched entries
    16  report kept entries
    32  report dropped entries

If you want all information, use `-V` as *first* argument.  Surround a
pattern with `-V` and `-V0` to get all information about this pattern
only.


Examples
========

The following examples run the command `env`, which prints its
environment.

 1. Blacklisting: Drop all entries whose name starts with `HTTP` or
    `FTP`.  Note, that globs need to match the entire name,

        $ fenv    'HTTP*' 'FTP*' -- env    # -G (glob) is the default
        $ fenv -E '^(HTTP|FTP)'  -- env

    while regular expressions match on substrings without anchoring.

 2. Whitelisting: Keep only the entries named `HOME` and `USER`:

        $ fenv -k HOME USER -- env

 3. Exceptions: Drop all entries whose name starts with `MC_`, except
    for `MC_XDG_OPEN`:

        $ fenv -k MC_XDG_OPEN -d 'MC_*' -- env

 4. Pattern with leading dash: Drop all environment variables whose
    value starts with a dash:

        $ fenv -v '--*' -- env

 5. Looking at values: Drop all variables with an empty value:

        $ fenv -v '' -- env

 6. Match name *and* value: Drop the PATH variable if it contains the
    word `foo`.

        $ fenv -w "PATH=*foo*" -- env
