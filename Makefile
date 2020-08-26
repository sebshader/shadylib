# Makefile for shadylib

lib.name = shadylib

# add your .c source files, one object per file, to the SOURCES
# variable, help files will be included automatically, and for GUI
# objects, the matching .tcl file too
class.sources =          moop~.c           prepender.c       shadylook~.c \
bpbuzz~.c         neadsr~.c         rcombf~.c         tabread4hs~.c \
buzz~.c           nead~.c           rectoratord~.c    tcheb~.c \
delreadc~.c       near~.c           rectorator~.c     transpose~.c \
delwritec~.c      nrcombf~.c        rminus~.c         triangulatord~.c \
dsrand~.c         operatord~.c      rootinfo.c        triangulator~.c \
fmod~.c           operator~.c       rover~.c          updel.c \
highest~.c        phasorator~.c     sampphase~.c      upmet.c \
inrange.c         pib~.c            scaler.c          vdhs~.c \
pinb~.c           scaler~.c         voisim~.c 		  log2.c \
modf~.c           powclip~.c        shadylook.c		lag~.c \
multatord~.c	  siglinterp~.c		realpass~.c		noteson.c

# list all pd objects (i.e. myobject.pd) files here, and their helpfiles will
# be included automatically
datafiles = \
	lmap.pd_lua \
	$(wildcard *.pd) \
	$(wildcard *.txt) \
	$(empty)

datadirs = \
	manual

shared.sources = \
	libshadylib.c

# .. broke after 0.6.0
cflags = -DVERSION='"$(lib.version)"'

DATE_FMT = %Y/%m/%d at %H:%M:%S UTC
ifdef SOURCE_DATE_EPOCH
    BUILD_DATE ?= $(shell date -u -d "@$(SOURCE_DATE_EPOCH)" "+$(DATE_FMT)" 2>/dev/null || date -u -r "$(SOURCE_DATE_EPOCH)" "+$(DATE_FMT)" 2>/dev/null || date -u "+$(DATE_FMT)")
endif
ifdef BUILD_DATE
cflags += -DBUILD_DATE='"$(BUILD_DATE)"'
endif

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

install: install-examples

install-examples: all
	$(INSTALL_DIR) $(installpath)/examples/deprecated
	$(INSTALL_DATA) $(wildcard examples/deprecated/*) \
		$(installpath)/examples/deprecated
	$(INSTALL_DATA) $(wildcard examples/*.tcl examples/*.pd examples/*.pd_lua) \
		$(installpath)/examples
