# Makefile \
This software is copyrighted by IOhannes m zmölnig.  The following \
terms (the "Standard Improved BSD License") apply to all files associated with \
the software unless explicitly disclaimed in individual files: \
\
Redistribution and use in source and binary forms, with or without \
modification, are permitted provided that the following conditions are \
met: \
\
1. Redistributions of source code must retain the above copyright \
   notice, this list of conditions and the following disclaimer. \
2. Redistributions in binary form must reproduce the above \
   copyright notice, this list of conditions and the following \
   disclaimer in the documentation and/or other materials provided \
   with the distribution. \
3. The name of the author may not be used to endorse or promote \
   products derived from this software without specific prior \
   written permission. \
\
THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY \
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, \
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A \
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR \
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, \
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED \
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, \
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND \
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT \
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING \
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF \
THE POSSIBILITY OF SUCH DAMAGE. \
(c) 2006 IOhannes m zmölnig \

# path to pd
## change this according to your setup!
PDROOT=/Applications/Pd-extended.app/Contents/Resources

# here we find the sources of pd (and evtl. the pd.lib)
PDSRCDIR=/Applications/Pd-extended.app/Contents/Resources/include/pdextended
PDLIBDIR=/Applications/Pd-extended.app/Contents/Resources/bin

# this is the filename-extension
# people have to specify it at the cmdline: eg "make pd_linux"
EXTENSION=$(MAKECMDGOALS)

# if no filename-extension is supplied by the user
# try to guess one, based on what "uname" tells us
UNAME := $(shell uname -s)
ifeq ($(UNAME),Linux)
  DEFAULTEXTENSION= pd_linux
else
  ifeq ($(UNAME),Darwin)
    DEFAULTEXTENSION= pd_darwin
  else
    ifeq (MINGW,$(findstring MINGW,$(UNAME)))
      DEFAULTEXTENSION= pd_nt
    else
      ifeq ($(UNAME),IRIX)
	UNAMEV := $(shell uname -R)
	ifeq (6.,$(findstring 6.,$(UNAMEV)))
	  DEFAULTEXTENSION= pd_irix6
	else
	  DEFAULTEXTENSION= pd_irix5
	endif
      else
	DEFAULTEXTENSION=help
      endif
    endif
  endif
endif

# if no extension is given, call "make" again with a guessed extension
auto:
	make $(DEFAULTEXTENSION)

# just a stupid fallback
help: 
	@echo "choose one command:  make pd_linux (linux), make pd_darwin (osX), make pd_irix5 (IRIX5), make pd_irix6 (IRIX6), make dll (MSVC), make pd_nt (MinWG)"

# delete old build files
clean:
	-rm -f *.dll *.pd_* *.o *.obj *~

# we want to compile all C-files we find in the current directory
SOURCES=$(sort $(filter %.c, $(wildcard *.c)))
# each C-files maps will become an external with the given filename-extension
TARGETS=$(SOURCES:.c=.$(EXTENSION))


# ----------------------- Linux -----------------------

pd_linux: $(TARGETS)

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

LINUXLDFLAGS =  -export_dynamic -shared  -lc -lm

LINUXINCLUDE =  -I$(PDSRCDIR) -ldns_sd

%.pd_linux: %.c
	$(CC) $(LINUXLDFLAGS) $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.pd_linux $*.c
	strip --strip-unneeded $*.pd_linux



# ----------------------- Mac OSX -----------------------

pd_darwin: $(TARGETS)

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch -arch x86_64 -arch i386

DARWININCLUDE = -I$(PDSRCDIR)

DARWINLDFLAGS = -bundle -undefined suppress -flat_namespace

%.pd_darwin: %.c
	$(CC) $(DARWINCFLAGS) $(DARWININCLUDE) $(DARWINLDFLAGS) -o $*.pd_darwin $*.c


# ----------------------- IRIX 5.x -----------------------
pd_irix5: $(TARGETS)

SGICFLAGS5 = -o32 -DPD -DSGI -O2

SGIINCLUDE =  -I$(PDSRCDIR) -ldns_sd

SGILDFLAGS =  -elf -shared -rdata_shared

%.pd_irix5: %.c
	$(CC) $(SGICFLAGS5) $(SGIINCLUDE) -o $*.o -c $*.c
	$(LD) $(SGILDFLAGS) -o $*.pd_irix5 $*.o
	rm $*.o


# ----------------------- IRIX 6.x -----------------------
pd_irix6: $(TARGETS)

SGICFLAGS6 = -DPD -DSGI -n32 \
	-OPT:roundoff=3 -OPT:IEEE_arithmetic=3 -OPT:cray_ivdep=true \
	-Ofast=ip32

%.pd_irix6: %.c
	$(CC) $(SGICFLAGS6) $(SGIINCLUDE) -o $*.o -c $*.c
	$(LD) $(SGILDFLAGS) -o $*.pd_irix6 $*.o
	rm $*.o


# ----------------------- NT -----------------------
dll: $(TARGETS)

PDNTCFLAGS = /W3 /WX /DPD /DNT /D__WIN32__ /DMSW /nologo

VC="C:\Programme\Microsoft Visual Studio\Vc98"

PDNTINCLUDE = /I. /I$(PDROOT)\tcl\include /I$(PDSRCDIR)\src /I$(VC)\include

PDNTLDIR = $(VC)\lib

PDNTLIB = $(PDNTLDIR)\libc.lib \
	$(PDNTLDIR)\oldnames.lib \
	$(PDNTLDIR)\kernel32.lib \
	$(PDLIBDIR)\pd.lib 

%.dll: %.c
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /dll /export:$*_setup $*.obj $(PDNTLIB)


pd_nt: $(TARGETS)

MINGWCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch -mms-bitfields

MINGWLDFLAGS =  -export_dynamic -shared -lm -lkernel32 -lcoldname -lcrtdll -lpd -L$(PDLIBDIR)

MINGWINCLUDE =  -I$(PDSRCDIR)

%.pd_nt: %.c
	$(CC) $(MINGWLDFLAGS) $(MINGWCFLAGS) $(MINGWINCLUDE) -o $*.dll $*.c
