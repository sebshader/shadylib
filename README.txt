This is a pure data library

Many abstractions and externals are just ports and modifications of existing
programs, primarily patches from the pure data help files.

This library was mainly developed against Pd-Extended-43.4

Various pure pd abstractions are in the purepdnfun folder within the examples
folder, which also holds my tcl plugin file for Pd-extended.

to install just put this library in your search path and use [import shadylib]

I would have liked to release everything under a BSD license but don't
think I can using externals that are licensed as GPL.
problematic .c files: exponential envelopes, buzz~, and the makefile

documentation for the sequencer is in manual/sequencerdoc.txt
there is a summary in manual/Overview.pd

requires pdlua for lmap, as well as various libraries included in Pd-Extended
43.4