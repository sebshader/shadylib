# this is the UNIX-style complicated layout dir, simple goes to $(prefix)/po
NAME = numargs
PDINCLUDE = -I/Applications/Pd-Extended.app/Contents/Resources/include


# ----------------------- NT -----------------------

pd_nt: $(NAME).dll

.SUFFIXES: .dll

PDNTCFLAGS = /W3 /WX /DNT /DPD /nologo -D_CRT_SECURE_NO_WARNINGS \
    -D_CRT_NONSTDC_NO_DEPRECATE
# VC="C:\Program Files\Microsoft Visual Studio\Vc98"
VC = "C:\Program Files\Microsoft Visual Studio 9.0\VC"
VSTK = "C:\Program Files\Microsoft SDKs\Windows\v6.0A"

PDNTINCLUDE = /I. /I..\..\src /I$(VC)\include

PDNTLDIR = $(VC)\lib
PDNTLIB = /NODEFAULTLIB:libcmt /NODEFAULTLIB:oldnames /NODEFAULTLIB:kernel32 \
	$(PDNTLDIR)\libcmt.lib $(PDNTLDIR)\oldnames.lib \
        $(VSTK)\lib\kernel32.lib \
	 ..\..\bin\pd.lib 

.c.dll:
	cl $(PDNTCFLAGS) $(PDNTINCLUDE) /c $*.c
	link /nologo /dll /export:$(CSYM)_setup $*.obj $(PDNTLIB)

# ----------------------- LINUX i386 -----------------------

pd_linux: $(NAME).pd_linux

.SUFFIXES: .pd_linux

LINUXCFLAGS = -DPD -O2 -funroll-loops -fomit-frame-pointer -fPIC \
    -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch $(CFLAGS)

LINUXINCLUDE =  -I../../src

.c.pd_linux:
	$(CC) $(LINUXCFLAGS) $(LINUXINCLUDE) -o $*.o -c $*.c
	$(CC) -shared -o $*.pd_linux $*.o -lc -lm
	rm -f $*.o

# ----------------------- Mac OSX -----------------------

pd_darwin: $(NAME).pd_darwin

.SUFFIXES: .pd_darwin

DARWINCFLAGS = -DPD -O2 -Wall -W -Wshadow -Wstrict-prototypes \
    -Wno-unused -Wno-parentheses -Wno-switch

.c.pd_darwin:
	$(CC) $(DARWINCFLAGS) $(PDINCLUDE) -o $*.o -c $*.c
	$(CC) -bundle -undefined suppress -flat_namespace -o $*.pd_darwin $*.o 
	rm -f $*.o


# ----------------------------------------------------------

distclean clean: clean-externals
	rm -f *.o *.pd_* *.l_* *.d_* *.m_* so_locations

# ----------------------------------------------------------
# ----------------------------------------------------------