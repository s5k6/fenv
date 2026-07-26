
fenv --- run a program in a modified environment
================================================

Similar to env(1), but more focused on filtering environment
variables.  `fenv` = “filter env”.

Environment variables can be filterd using regular expressions, globs,
or fixed strings, each of them matching against name or value (or
both) of an entry in te environment.  Whitelisting and blacklisting
are straight-forward.

See [the manual][1] for instructions.


[1]: ./help.txt
