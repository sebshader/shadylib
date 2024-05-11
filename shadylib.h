#ifndef INCLUDE_SHADYLIB_H__
#define INCLUDE_SHADYLIB_H__

#include "m_pd.h"

#ifdef _WIN32
#ifdef SHADYLIB_SHARED
#define INTERN __declspec(dllexport) extern
#else
#define INTERN __declspec(dllimport) extern
#endif /* SHADYLIB_SHARED */
#else
#define INTERN extern
#endif /* _WIN32 */

/* this seems faster than fmin/fmax sometimes bc of type conversion */
#define shadylib_min(X, Y)  ((X) < (Y) ? (X) : (Y))
#define shadylib_max(X, Y)  ((X) > (Y) ? (X) : (Y))
/* clamp x between y and z */
#define shadylib_clamp(X, Y, Z) ((X) > (Z) ? (Z) : ((X) < (Y) ? (Y) : (X)))

#if PD_FLOATSIZE == 64
#include <math.h>
#define shadylib_pdfloat_abs(X) fabs(X)
#else
#include <math.h>
#define shadylib_pdfloat_abs(X) fabsf(X)
#endif

#ifdef __GNUC__
    #define SHADYLIB_UNUSED(x) UNUSED_ ## x __attribute__((__unused__))
    #define SHADYLIB_UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED ## x
    #if __GNUC__ >= 9
    #pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
#else
    #define SHADYLIB_UNUSED(x) UNUSED_ ## x
    #define SHADYLIB_UNUSED_FUNCTION(x) UNUSED ## x
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define SHADYLIB_UNUSED_ATTRIBUTE __attribute__((unused))
#else
    #define SHADYLIB_UNUSED_ATTRIBUTE
#endif

#define SHADYLIB_UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

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
# define SHADYLIB_HIOFFSET 1
# define SHADYLIB_LOWOFFSET 0
#else
# define SHADYLIB_HIOFFSET 0    /* word offset to find MSB */
# define SHADYLIB_LOWOFFSET 1    /* word offset to find LSB */
#endif

#define SHADYLIB_TRUE 1
#define SHADYLIB_FALSE 0

#define SHADYLIB_NORMHIPART 1094189056 //will this work on big-endian?

union shadylib_tabfudge
{
    double tf_d;
    int32_t tf_i[2];
};

/* union for setting floats or receiving signals */
union shadylib_floatpoint
{
    t_sample *vec;
    t_float val;
};

typedef struct _shadylib_oscctl
{
    union shadylib_floatpoint invals[2];
    int num; /* number of msg inlets: 0, 1, or 2
        0 is all signals, 1 is an add inlet,
        2 is a multiply inlet and an add inlet */
} t_shadylib_oscctl;

typedef struct _shadylib_delwritectl
{
    int c_n;
    t_sample *c_vec;
    int c_phase;
} t_shadylib_delwritectl;

typedef struct _shadylib_sigdelwritec
{
    t_object x_obj;
    t_symbol *x_sym;
    t_float x_deltime;  /* delay in msec (added by Mathieu Bouchard) */
    t_shadylib_delwritectl x_cspace;
    int x_sortno;   /* DSP sort number at which this was last put on chain */
    int x_rsortno;  /* DSP sort # for first delread or write in chain */
    int x_vecsize;  /* vector size for delread~ to use */
    t_float x_f;
} t_shadylib_sigdelwritec;

/* ugen_getsortno in pd binary <= 0.51.3 at least */
EXTERN int ugen_getsortno(void);
INTERN void shadylib_sigdelwritec_checkvecsize(t_shadylib_sigdelwritec *x, int vecsize);
INTERN void shadylib_sigdelwritec_updatesr (t_shadylib_sigdelwritec *x, t_float sr);
#define SHADYLIB_XTRASAMPS 4
#define SHADYLIB_SAMPBLK 4
#define SHADYLIB_DEFDELVS 64  /* LATER get this from canvas at DSP time */

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
} shadylib_t_flint;

/* check if floating point number is denormal */

//#define IS_DENORMAL(f) (((*(unsigned int *)&(f))&0x7f800000) == 0)

#define SHADYLIB_IS_NOT_DENORMAL(f) (((((shadylib_t_flint)(f)).i) & 0x7f800000) != 0)

#elif PD_FLOAT_PRECISION == 64

typedef union
{
    unsigned int i[2];
    t_float f;
} shadylib_t_flint;

/* Hope HIOFFSET works alright here.. */
#define SHADYLIB_IS_NOT_DENORMAL(f) (((((shadylib_t_flint)(f)).i[SHADYLIB_HIOFFSET]) & 0x7ff00000) != 0)

#endif // endif PD_FLOAT_PRECISION

#else   // if not defined(__i386__) || defined(__x86_64__)
#define SHADYLIB_IS_NOT_DENORMAL(f) 1
#endif // end if defined(__i386__) || defined(__x86_64__)

INTERN void shadylib_checkalign(void);

/* exponential range for envelopes is 60dB */
#define SHADYLIB_ENVELOPE_RANGE 0.001
#define SHADYLIB_ENVELOPE_MAX   (1.0 - SHADYLIB_ENVELOPE_RANGE)

typedef struct _shadylib_stage {
    t_float lin; // 0.001 for fully exponential (60 db), 1.0 for fully linear
    t_float op;  // geometric multiplier or linear subtraction (if lin == 1.0)
    t_float base;// addition for each stage
    t_int nsamp; // # of samples in stage
} t_shadylib_stage;

INTERN t_class *shadylib_sigdelwritec_class;

INTERN t_int shadylib_ms2samps(t_float time, t_float sr);
INTERN void shadylib_f2axfade (t_float a, t_shadylib_stage *stage, int samesamp);
INTERN void shadylib_ms2axfade (t_shadylib_stage *stage);
INTERN void shadylib_f2dxfade(t_float a, t_shadylib_stage *stage, int samesamp);
INTERN void shadylib_ms2dxfade (t_shadylib_stage *stage);
INTERN void shadylib_f2rxfade(t_float a, t_shadylib_stage *stage, int samesamp);
INTERN void shadylib_ms2rxfade (t_shadylib_stage *stage);

#define SHADYLIB_TABLESIZE 2048 /* size of tables in shadylook~ */

typedef enum _tabtype {
    REXP,
    GAUS,
    CAUCH
} t_shadylib_tabtype;

INTERN t_float shadylib_readtab(t_shadylib_tabtype type, t_float index);

INTERN void shadylib_maketabs(void);
INTERN void shadylib_freetabs(t_class *dummy);

/*buzz stuff */
INTERN void shadylib_makebuzz(void);
INTERN void shadylib_freebuzz(t_class *dummy);

INTERN t_int *shadylib_opd_perf0(t_int *w);
INTERN t_int *shadylib_opd_perf1(t_int *w);
INTERN t_int *shadylib_opd_perf2(t_int *w);

INTERN t_int *shadylib_recd_perf0(t_int *w);
INTERN t_int *shadylib_recd_perf1(t_int *w);
INTERN t_int *shadylib_recd_perf2(t_int *w);

INTERN t_int *shadylib_trid_perf0(t_int *w);
INTERN t_int *shadylib_trid_perf1(t_int *w);
INTERN t_int *shadylib_trid_perf2(t_int *w);

INTERN t_sample *shadylib_sintbl;
INTERN t_sample *shadylib_cosectbl;

/* used in the cosecant table for values very close to 1/0 */
#define SHADYLIB_BADVAL 1e20f

/* size of tables */
#define SHADYLIB_BUZZSIZE 8192

/* maximum harmonics for small frequencies */
#define SHADYLIB_MAXHARM ((int)((4294967295/(2*SHADYLIB_BUZZSIZE)) - 2))

/* memset */

#ifdef _WIN32
# include <malloc.h> /* MSVC or mingw on windows */
#elif defined(__linux__) || defined(__APPLE__)
# include <alloca.h> /* linux, mac, mingw, cygwin */
#else
# include <stdlib.h> /* BSDs for example */
#endif

#ifndef HAVE_ALLOCA /* can work without alloca() but we never need it */
#define HAVE_ALLOCA 1
#endif

/* bigger that this we use alloc, not alloca */
#define SHADYLIB_LIST_NGETBYTE 100

/* -------------- utility functions: storage, copying  -------------- */
    /* List element for storage.  Keep an atom and, in case it's a pointer,
        an associated 'gpointer' to protect against stale pointers. */
typedef struct _shadylib_listelem
{
    t_atom l_a;
    t_gpointer l_p;
} t_shadylib_listelem;

typedef struct _shadylib_alist
{
    t_pd l_pd;          /* object to point inlets to */
    t_shadylib_listelem *l_vec;  /* pointer to items */
    int l_n;            /* number of items */
    int l_npointer;     /* number of pointers */
} t_shadylib_alist;

#if HAVE_ALLOCA
#define SHADYLIB_ATOMS_ALLOCA(x, n) ((x) = (t_atom *)((n) < \
    SHADYLIB_LIST_NGETBYTE ? alloca((n) * sizeof(t_atom)) : \
    getbytes((n) * sizeof(t_atom))))
#define SHADYLIB_ATOMS_FREEA(x, n) ( \
    ((n) < SHADYLIB_LIST_NGETBYTE || (freebytes((x), (n) * sizeof(t_atom)), 0)));
#else
#define SHADYLIB_ATOMS_ALLOCA(x, n) ((x) = (t_atom *)getbytes((n) * \
        sizeof(t_atom)))
#define SHADYLIB_ATOMS_FREEA(x, n) (freebytes((x), (n) * sizeof(t_atom)))
#endif

INTERN void shadylib_atoms_copy(int argc, t_atom *from, t_atom *to);
INTERN t_class *shadylib_alist_class;
INTERN void shadylib_alist_init(t_shadylib_alist *x);
INTERN void shadylib_alist_clear(t_shadylib_alist *x);
INTERN void shadylib_alist_copyin(t_shadylib_alist *x, t_symbol *s, int argc, t_atom *argv,
    int where);
INTERN void shadylib_alist_list(t_shadylib_alist *x, t_symbol *s, int argc, t_atom *argv);
INTERN void shadylib_alist_anything(t_shadylib_alist *x, t_symbol *s, int argc, t_atom *argv);
INTERN void shadylib_alist_toatoms(t_shadylib_alist *x, t_atom *to, int onset, int count);
INTERN void shadylib_alist_clone(t_shadylib_alist *x, t_shadylib_alist *y, int onset, int count);
INTERN void shadylib_alist_setup(void);
INTERN int shadylib_atoms_eq(t_atom *first, t_atom *second);

//linear congruential method from "Algorithms in C" by Robert Sedgewick
#define m 16777216 /* 2^24: 24 digits are 0 */
#define m1 4096 /* 2^12 */
/*#define b 2375621 arbitrary number ending in ...(even digit)21
                       with 1 digit less than m */

/*below is a program to test the number generator
//chisquare: test if within 128 (2*sqrt(4096)) of 4096

#include <stdio.h>

#define m 16777216
#define m1 4096
#define b 2375621

//you can (and should) substitute 60000 with other values greater than 40960
#define N 60000

int mult(int p, int q) {
    int p1, p0, q1, q0;
    p1 = p/m1; p0 = p%m1;
    q1 = q/m1; q0 = q%m1;
    return (((p0*q1 + p1*q0)%m1)*m1 + p0*q0)%m;
}

int randomtest(int in) {
    in = (mult(in, b) + 1)%m;
    return in;
}

int tab[m1];

int main (int argc, char **argv) {
    int state = 0, i;
    float result;
    for(i = 0; i < m1; i++) tab[i] = 0;
    for(i = 0; i < N; i++) {
        state = randomtest(state);
        tab[state/m1]++;
    }
    state = 0;
    for(i = 0; i < m1; i++) {
        state += tab[i]*tab[i];
    }
    printf("state = %i\n", state);
    result = (((float)state)/N)*m1 - N;
    printf("result = %f\n", result);
    printf("difference: %f (absolute value should be < 128)\n", m1 - result);
}

*/

/* 579 = b/m1, 4037 = b%m1 */
inline int mult(unsigned int p) {
    unsigned int p1, p0;
    p1 = p/m1; p0 = p%m1;
    return (((p0*579 + p1*4037)%m1)*m1 + p0*4037)%m;
}

inline unsigned int randlcm(unsigned int in) {
    return (mult(in) + 1) & (m - 1);
}

#undef m
#undef m1

#endif /* INCLUDE_SHADYLIB_H__ */
