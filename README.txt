to make & install: 1. check PD_PATH variable in Makefile to point towards
the correct Resource/source directory for pd.
2. type "make install" in this (shadylib) directory

This is a pure data library

Many abstractions and externals are just ports and modifications of existing
programs, primarily patches from the pure data help files.

Some external libraries are needed, primarily zexy and iemlib

I would have liked to release everything under a BSD license but don't
think I can using externals that are licensed as GPL.
problematic .c files: exponential envelopes, buzz~, and the makefile

documentation for the sequencer is in manual/sequencerdoc.txt
there is a summary in manual/Overview.pd

requires pdlua for lmap, as well as various libraries