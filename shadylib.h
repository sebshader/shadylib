#include "m_pd.h"
#include <math.h>

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

#ifdef IRIX
#include <sys/endian.h>
#endif

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
#include <machine/endian.h>
#endif

#if defined(__linux__) || defined(__CYGWIN__) || defined(__GNU__) || defined(ANDROID)
#include <endian.h>
#endif

#ifdef __MINGW32__
#include <sys/param.h>
#endif

#ifdef _MSC_VER
/* _MSVC lacks BYTE_ORDER and LITTLE_ENDIAN */
#define LITTLE_ENDIAN 0x0001
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)                         
#error No byte order defined                                                    
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
# define HIOFFSET 1     
# define LOWOFFSET 0                                                       
#else                                                                           
# define HIOFFSET 0    /* word offset to find MSB */                             
# define LOWOFFSET 1    /* word offset to find LSB */                            
#endif

#define TRUE 1
#define FALSE 0

#define NORMHIPART 1094189056 //will this work on big-endian?

union tabfudge
{
    double tf_d;
    int32_t tf_i[2];
};

/* union for setting floats or receiving signals */
union floatpoint
{
	t_sample *vec;
	t_float val;
};

typedef struct oscctl
{
	union floatpoint invals[2];
	int num; /* number of msg inlets: 0, 1, or 2 
		0 is all signals, 1 is an add inlet,
		2 is a multiply inlet and an add inlet */
} t_oscctl;

typedef struct delwritectl
{
    int c_n;
    t_sample *c_vec;
    int c_phase;
} t_delwritectl;

typedef struct _sigdelwritec
{
    t_object x_obj;
    t_symbol *x_sym;
    t_float x_deltime;  /* delay in msec (added by Mathieu Bouchard) */
    t_delwritectl x_cspace;
    int x_sortno;   /* DSP sort number at which this was last put on chain */
    int x_rsortno;  /* DSP sort # for first delread or write in chain */
    int x_vecsize;  /* vector size for delread~ to use */
    t_float x_f;
} t_sigdelwritec;

EXTERN int ugen_getsortno(void);
EXTERN void sigdelwritec_checkvecsize(t_sigdelwritec *x, int vecsize);
EXTERN void sigdelwritec_updatesr (t_sigdelwritec *x, t_float sr);
#define XTRASAMPS 4
#define SAMPBLK 4
#define DEFDELVS 64             /* LATER get this from canvas at DSP time */

EXTERN t_class *sigdelwritec_class;

/*
 *   utility functions used in pd EXTERNals 
 *   Copyright (c) 2000-2014 by Tom Schouten & Seb Shader
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef PD_FLOAT_PRECISION
#define PD_FLOAT_PRECISION 32
#endif

#if defined(__i386__) || defined(__x86_64__) // type punning code:

#if PD_FLOAT_PRECISION == 32

typedef union
{
    unsigned int i;
    t_float f;
} t_flint;

/* check if floating point number is denormal */

//#define IS_DENORMAL(f) (((*(unsigned int *)&(f))&0x7f800000) == 0) 

#define IS_DENORMAL(f) (((((t_flint)(f)).i) & 0x7f800000) == 0)

#elif PD_FLOAT_PRECISION == 64

typedef union
{
    unsigned int i[2];
    t_float f;
} t_flint;

#define IS_DENORMAL(f) (((((t_flint)(f)).i[1]) & 0x7ff00000) == 0)

#endif // endif PD_FLOAT_PRECISION

#else   // if not defined(__i386__) || defined(__x86_64__)
#define IS_DENORMAL(f) 0
#endif // end if defined(__i386__) || defined(__x86_64__)

EXTERN void checkalign(void);

/* exponential range for envelopes is 60dB */
#define ENVELOPE_RANGE 0.001
#define ENVELOPE_MAX   (1.0 - ENVELOPE_RANGE)

typedef struct _stage {
	t_float lin; // 0.001 for fully exponential (60 db), 1.0 for fully linear
	t_float op;  // geometric multiplier or linear subtraction (if lin == 1.0)
	t_float base;// addition for each stage
	t_int nsamp; // # of samples in stage
} t_stage;     

EXTERN t_class *sigdelwritec_class;

EXTERN t_int ms2samps(t_float time, t_float sr);
EXTERN void f2axfade (t_float a, t_stage *stage, int samesamp);
EXTERN void ms2axfade (t_stage *stage);
EXTERN void f2dxfade(t_float a, t_stage *stage, int samesamp);
EXTERN void ms2dxfade (t_stage *stage);
EXTERN void f2rxfade(t_float a, t_stage *stage, int samesamp);
EXTERN void ms2rxfade (t_stage *stage);

#define SHABLESIZE 2048 /* size of tables in shadylook~ */

typedef enum _tabtype {
	REXP,
	GAUS,
	CAUCH
} t_tabtype;

EXTERN t_float readtab(t_tabtype type, t_float index);

EXTERN void maketabs(void);

/*buzz stuff */
EXTERN void makebuzz(void);

EXTERN t_int *opd_perf0(t_int *w);
EXTERN t_int *opd_perf1(t_int *w);
EXTERN t_int *opd_perf2(t_int *w);

EXTERN t_int *recd_perf0(t_int *w);
EXTERN t_int *recd_perf1(t_int *w);
EXTERN t_int *recd_perf2(t_int *w);

EXTERN t_int *trid_perf0(t_int *w);
EXTERN t_int *trid_perf1(t_int *w);
EXTERN t_int *trid_perf2(t_int *w);

EXTERN unsigned char aligned;
EXTERN t_sample *sintbl;
EXTERN t_sample *cosectbl;

/* used in the cosecant table for values very close to 1/0 */
#define BADVAL 1e20f

/* size of tables */
#define BUZZSIZE 8192

/* maximum harmonics for small frequencies */
#define MAXHARM ((int)((4294967295/(2*BUZZSIZE)) - 2))

#if HAVE_ALLOCA
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)((n) < LIST_NGETBYTE ?  \
        alloca((n) * sizeof(t_atom)) : getbytes((n) * sizeof(t_atom))))
#define ATOMS_FREEA(x, n) ( \
    ((n) < LIST_NGETBYTE || (freebytes((x), (n) * sizeof(t_atom)), 0)))
#else
#define ATOMS_ALLOCA(x, n) ((x) = (t_atom *)getbytes((n) * sizeof(t_atom)))
#define ATOMS_FREEA(x, n) (freebytes((x), (n) * sizeof(t_atom)))
#endif