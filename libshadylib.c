#include <math.h>
#define SHADYLIB_SHARED
#include "shadylib.h"

t_float *shadylib_gaustab; /* e^(-x^2) */
t_float *shadylib_couchtab; /* 1/(x^2 + 1) */
t_float *shadylib_rexptab; /* (e^(-x) - .001)/.999 to x=ln(.001) */
t_sample *shadylib_sintbl;
t_sample *shadylib_cosectbl;
static unsigned char aligned = 0; //doesn't have to be fast

INTERN t_float shadylib_readtab(t_shadylib_tabtype type, t_float index) {
    t_float *tab = shadylib_rexptab + (type * SHADYLIB_TABLESIZE);
    int iindex;
    t_float frac, index2;
    index *= SHADYLIB_TABLESIZE;
    index = shadylib_max(index, 0);
    if (index >= SHADYLIB_TABLESIZE - 1) return tab[SHADYLIB_TABLESIZE - 1];
    else {
        iindex = index;
        frac = index - iindex;
        index = tab[iindex++];
        index2 = tab[iindex] - index;
#ifdef FP_FAST_FMAF
        return fmaf(frac, index2, index);
#else
        return index2*frac + index;
#endif
    }
}

INTERN void shadylib_maketabs(void) {
    double lnths;
    t_float incr = 4.0/(SHADYLIB_TABLESIZE - 1);
    t_float sqr;
    if (shadylib_rexptab) return;
    shadylib_rexptab = getbytes(sizeof(t_float)*SHADYLIB_TABLESIZE*3);
    shadylib_gaustab = shadylib_rexptab + SHADYLIB_TABLESIZE;
    shadylib_couchtab = shadylib_gaustab + SHADYLIB_TABLESIZE;
    for(int i = 0; i < SHADYLIB_TABLESIZE; i++) {
        lnths = (i/(double)(SHADYLIB_TABLESIZE - 1))*log(.001);
        shadylib_rexptab[i] = (exp(lnths) - .001)/.999;
        sqr = incr*i;
        shadylib_gaustab[i] = exp(-(sqr*sqr));
        sqr = sqr*8;
        sqr *= sqr;
        shadylib_couchtab[i] = 1/(sqr + 1);
    }
    /* fix shadylib_rexptab */
    shadylib_rexptab[SHADYLIB_TABLESIZE - 1] = 0;
}

INTERN void shadylib_freetabs(t_class* SHADYLIB_UNUSED(dummy)) {
    if(shadylib_rexptab) {
        freebytes(shadylib_rexptab, sizeof(t_float)*SHADYLIB_TABLESIZE*3);
        shadylib_rexptab = NULL;
    }
}

/* create sine and cosecant tables */
/* copied symmetric table pr: https://github.com/pure-data/pure-data/pull/106 */
INTERN void shadylib_makebuzz(void) {
    double phsinc = 2*M_PI/SHADYLIB_BUZZSIZE;
    int i;
    t_sample *sinPtr;
    if(shadylib_sintbl) return;

    shadylib_sintbl = (t_sample *)getbytes(sizeof(t_sample) * (SHADYLIB_BUZZSIZE + 1)*2);
    sinPtr = shadylib_sintbl;
    shadylib_cosectbl = shadylib_sintbl + SHADYLIB_BUZZSIZE + 1;
    for(i = 0; i < SHADYLIB_BUZZSIZE/4; sinPtr++, i++) {
        *sinPtr = (t_sample)sin(i*phsinc);
    }
    *sinPtr++ = 1.;
    for (i = SHADYLIB_BUZZSIZE/4; i--; sinPtr++)
        *sinPtr = shadylib_sintbl[i];
    for (i = SHADYLIB_BUZZSIZE/2; i--; sinPtr++)
        *sinPtr = -shadylib_sintbl[i];
    for (i = 0; i <= SHADYLIB_BUZZSIZE; i++) {
        shadylib_cosectbl[i] = 1/shadylib_sintbl[i];
    }

    /* insert SHADYLIB_BADVALs */
    shadylib_cosectbl[0] = shadylib_cosectbl[SHADYLIB_BUZZSIZE/2] = shadylib_cosectbl[SHADYLIB_BUZZSIZE] = SHADYLIB_BADVAL;
    for (i=1; i<=8; ++i) {
        shadylib_cosectbl[i] = shadylib_cosectbl[SHADYLIB_BUZZSIZE-i] = SHADYLIB_BADVAL;
        shadylib_cosectbl[SHADYLIB_BUZZSIZE/2-i] = shadylib_cosectbl[SHADYLIB_BUZZSIZE/2+i] = SHADYLIB_BADVAL;
    }
}

INTERN void shadylib_freebuzz(t_class* SHADYLIB_UNUSED(dummy)) {
    if(shadylib_sintbl) {
        freebytes(shadylib_sintbl,
            sizeof(t_sample) * (SHADYLIB_BUZZSIZE + 1)*2);
        shadylib_sintbl = NULL;
    }
}

INTERN t_float shadylib_scalerange (t_float a) {
    return (a - SHADYLIB_ENVELOPE_RANGE)/SHADYLIB_ENVELOPE_MAX;
}

INTERN t_float shadylib_ain2reala(t_float a) {
    return exp2(log2(SHADYLIB_ENVELOPE_RANGE)*(1 - a));
}

INTERN t_int shadylib_ms2samps(t_float time, t_float sr)
{
    t_int samp = (t_int)(sr*time/1000.0);
    if(samp < 1) samp = 1;
    return samp;
}

INTERN void shadylib_f2axfade (t_float a, t_shadylib_stage *stage, int samesamp) {
    a = shadylib_clamp(a, 0.f, 1.f);
    if(a != 1.f) { /*exponential*/
        a = shadylib_ain2reala(a);
        if(stage->lin == a && samesamp) return;
        stage->lin = a;
        stage->op = exp2(log2(a)/stage->nsamp);
        stage->base = (1 - stage->op)/(1 - a);
    } else { /*linear*/
        stage->lin = 1.f;
        stage->op = 1.f;
        stage->base = 1.0/stage->nsamp;
    }
}

INTERN void shadylib_ms2axfade (t_shadylib_stage *stage) {
    if (stage->lin != 1.f) {
        stage->op = exp2(log2(stage->lin)/stage->nsamp);
        stage->base = (1 - stage->op)/(1 - stage->lin);
    } else {
        stage->base = 1.0/stage->nsamp;
        stage->op = 1.f; //linear
    }
}

INTERN void shadylib_f2dxfade(t_float a, t_shadylib_stage *stage, int samesamp) {
    a = shadylib_clamp(a, 0.f, 1.f);
    if(a != 1.f) {/*exponential*/
        a = shadylib_ain2reala(a);
        if(stage->lin == shadylib_scalerange(a) && samesamp) return;
        stage->lin = shadylib_scalerange(a);
        stage->op = exp2(log2(a)/stage->nsamp);
        stage->base = (1 - stage->op)/(1 - stage->lin);
    } else {/*linear*/
        stage->lin = 1.f;
        stage->base = 1.0/stage->nsamp;
        stage->op = 1.f;
    }
}

INTERN void shadylib_ms2dxfade (t_shadylib_stage *stage) {
    if(stage->lin != 1.f) {
        stage->op = exp2(log2(stage->lin*SHADYLIB_ENVELOPE_MAX + SHADYLIB_ENVELOPE_RANGE)/stage->nsamp);
        stage->base = (1 - stage->op)/(1 - stage->lin);
    } else {
        stage->base = 1.0/stage->nsamp;
        stage->op = 1.f;
    }
}

INTERN void shadylib_f2rxfade(t_float a, t_shadylib_stage *stage, int samesamp) {
    a = shadylib_clamp(a, 0.f, 1.f);
    if(a != 1.f) {/*exponential*/
        a = shadylib_ain2reala(a);
        if(stage->lin == shadylib_scalerange(a) && samesamp) return;
        stage->lin = shadylib_scalerange(a);
        stage->op = exp2(log2(a)/stage->nsamp);
        stage->base = stage->lin*(stage->op - 1)/(1 - stage->lin);
    } else {/*linear*/
        stage->lin = 1.f;
        stage->base = -1.0/stage->nsamp;
        stage->op = 1.f;
    }
}

INTERN void shadylib_ms2rxfade (t_shadylib_stage *stage) {
    if(stage->lin != 1.f) {
        stage->op = exp2(log2(stage->lin*SHADYLIB_ENVELOPE_MAX + SHADYLIB_ENVELOPE_RANGE)/stage->nsamp);
        stage->base = stage->lin*(stage->op - 1)/(1 - stage->lin);
    } else {
        stage->base = -1.0/stage->nsamp;
        stage->op = 1.f;
    }
}

/* needed for delay objects */
t_class *shadylib_sigdelwritec_class;

INTERN void shadylib_sigdelwritec_updatesr (t_shadylib_sigdelwritec *x, t_float sr) /* added by Mathieu Bouchard */
{
    int nsamps = x->x_deltime * sr * 0.001f;
    if (nsamps < 1) nsamps = 1;
    nsamps += ((- nsamps) & (SHADYLIB_SAMPBLK - 1));
    nsamps += SHADYLIB_DEFDELVS;
    if (x->x_cspace.c_n != nsamps) {
      x->x_cspace.c_vec = (t_sample *)resizebytes(x->x_cspace.c_vec,
        (x->x_cspace.c_n + SHADYLIB_XTRASAMPS) * sizeof(t_sample),
        (         nsamps + SHADYLIB_XTRASAMPS) * sizeof(t_sample));
      x->x_cspace.c_n = nsamps;
      x->x_cspace.c_phase = SHADYLIB_XTRASAMPS;
    }
}

    /* routine to check that all delwrites/delreads/vds have same vecsize */
INTERN void shadylib_sigdelwritec_checkvecsize(t_shadylib_sigdelwritec *x, int vecsize)
{
    if (x->x_rsortno != ugen_getsortno())
    {
        x->x_vecsize = vecsize;
        x->x_rsortno = ugen_getsortno();
    }
    /*
        LATER this should really check sample rate and blocking, once that is
        supported.  Probably we don't actually care about vecsize.
        For now just suppress this check. */
#if 0
    else if (vecsize != x->x_vecsize)
        pd_error(x, "delwritec/delreadc/vdhs vector size mismatch");
#endif
}

INTERN void shadylib_checkalign(void) {
    union shadylib_tabfudge tf;
    if(aligned) return;
    tf.tf_d = SHADYLIB_UNITBIT32 + 0.5;
    if ((unsigned)tf.tf_i[SHADYLIB_LOWOFFSET] != 0x80000000)
        bug("shadylib: unexpected machine alignment");
    else aligned = 1;
}

INTERN t_int *shadylib_opd_perf0(t_int *w) {
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *mul = x->invals[0].vec;
    t_sample *add = x->invals[1].vec;
    t_sample *tab = shadylib_sintbl, *addr, f1, f2, frac;

    double dphase;
    int normhipart;
    union shadylib_tabfudge tf;

    tf.tf_d = SHADYLIB_UNITBIT32;
    normhipart = tf.tf_i[SHADYLIB_HIOFFSET];


        dphase = (double)(*in++ * (float)(SHADYLIB_BUZZSIZE)) + SHADYLIB_UNITBIT32;
        tf.tf_d = dphase;
        addr = tab + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
    while (--n)
    {
        dphase = (double)(*in++ * (float)(SHADYLIB_BUZZSIZE)) + SHADYLIB_UNITBIT32;
            frac = tf.tf_d - SHADYLIB_UNITBIT32;
        tf.tf_d = dphase;
            f1 = addr[0];
            f2 = addr[1];
        addr = tab + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + (*add++);
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
    }
            frac = tf.tf_d - SHADYLIB_UNITBIT32;
            f1 = addr[0];
            f2 = addr[1];
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + (*add++);
    return (w+5);
}

INTERN t_int *shadylib_opd_perf1(t_int *w) {
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *mul = x->invals[0].vec;
    t_float add = x->invals[1].val;
    t_sample *tab = shadylib_sintbl, *addr, f1, f2, frac;

    double dphase;
    int normhipart;
    union shadylib_tabfudge tf;

    tf.tf_d = SHADYLIB_UNITBIT32;
    normhipart = tf.tf_i[SHADYLIB_HIOFFSET];


        dphase = (double)(*in++ * (float)(SHADYLIB_BUZZSIZE)) + SHADYLIB_UNITBIT32;
        tf.tf_d = dphase;
        addr = tab + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
    while (--n)
    {
        dphase = (double)(*in++ * (float)(SHADYLIB_BUZZSIZE)) + SHADYLIB_UNITBIT32;
            frac = tf.tf_d - SHADYLIB_UNITBIT32;
        tf.tf_d = dphase;
            f1 = addr[0];
            f2 = addr[1];
        addr = tab + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + add;
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
    }
            frac = tf.tf_d - SHADYLIB_UNITBIT32;
            f1 = addr[0];
            f2 = addr[1];
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + add;
    return (w+5);
}

INTERN t_int *shadylib_opd_perf2(t_int *w) {
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_float mul = x->invals[0].val;
    t_float add = x->invals[1].val;
    t_sample *tab = shadylib_sintbl, *addr, f1, f2, frac;

    double dphase;
    int normhipart;
    union shadylib_tabfudge tf;

    tf.tf_d = SHADYLIB_UNITBIT32;
    normhipart = tf.tf_i[SHADYLIB_HIOFFSET];


        dphase = (double)(*in++ * (float)(SHADYLIB_BUZZSIZE)) + SHADYLIB_UNITBIT32;
        tf.tf_d = dphase;
        addr = tab + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
    while (--n)
    {
        dphase = (double)(*in++ * (float)(SHADYLIB_BUZZSIZE)) + SHADYLIB_UNITBIT32;
            frac = tf.tf_d - SHADYLIB_UNITBIT32;
        tf.tf_d = dphase;
            f1 = addr[0];
            f2 = addr[1];
        addr = tab + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*mul + add;
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
    }
            frac = tf.tf_d - SHADYLIB_UNITBIT32;
            f1 = addr[0];
            f2 = addr[1];
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*mul + add;
    return (w+5);
}

INTERN t_int *shadylib_recd_perf0(t_int *w) {
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *mul = x->invals[0].vec;
    t_sample *add = x->invals[1].vec;
    union shadylib_tabfudge inter;
    uint32_t casto;
    inter.tf_d = 1.0;
    while (n--)
    {
        casto = (uint32_t)(*in++ * (t_sample)4294967295);
        /* set the sign bit of double 1.0 */
        inter.tf_i[SHADYLIB_HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        *out++ = inter.tf_d*(*mul++) + (*add++);
    }
    return (w+5);
}

INTERN t_int *shadylib_recd_perf1(t_int *w) {
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *mul = x->invals[0].vec;
    t_float add = x->invals[1].val;
    union shadylib_tabfudge inter;
    uint32_t casto;
    inter.tf_d = 1.0;
    while (n--)
    {
        casto = (uint32_t)(*in++ * (t_sample)4294967295);
        inter.tf_i[SHADYLIB_HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        *out++ = inter.tf_d*(*mul++) + add;
    }
    return (w+5);
}

INTERN t_int *shadylib_recd_perf2(t_int *w) {
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_float mul = x->invals[0].val;
    t_float add = x->invals[1].val;
    union shadylib_tabfudge inter;
    uint32_t casto;
    inter.tf_d = 1.0;
    while (n--)
    {
        casto = (uint32_t)(*in++ * (t_sample)4294967295);
        inter.tf_i[SHADYLIB_HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        *out++ = inter.tf_d*mul + add;
    }
    return (w+5);
}

INTERN t_int *shadylib_trid_perf0(t_int *w) {
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *mul = x->invals[0].vec;
    t_sample *add = x->invals[1].vec;
    t_sample inter;
    uint32_t casto;
    while (n--)
    {
        casto = (uint32_t)(*in++ * 4294967296);
        if(casto & 2147483648) /* bit 31 */
            casto = ~casto;
        inter = (t_sample)casto/1073741823.5 - 1.f;
        #ifdef FP_FAST_FMAF
        *out++ = fmaf(inter, *mul++, *add++);
        #else
        *out++ = inter*(*mul++) + (*add++);
        #endif
    }
    return (w+5);
}

INTERN t_int *shadylib_trid_perf1(t_int *w) {
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *mul = x->invals[0].vec;
    t_float add = x->invals[1].val;
    t_sample inter;
    uint32_t casto;
    while (n--)
    {
        casto = (uint32_t)(*in++ * 4294967296);
        if(casto & 2147483648) /* bit 31 */
            casto = ~casto;
        inter = (t_sample)casto/1073741823.5 - 1.f;
        #ifdef FP_FAST_FMAF
        *out++ = fmaf(inter, *mul++, add);
        #else
        *out++ = inter*(*mul++) + add;
        #endif
    }
    return (w+5);
}

INTERN t_int *shadylib_trid_perf2(t_int *w) {
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_float mul = x->invals[0].val;
    t_float add = x->invals[1].val;
    t_sample inter;
    uint32_t casto;
    while (n--)
    {
        casto = (uint32_t)(*in++ * 4294967296);
        if(casto & 2147483648) /* bit 31 */
            casto = ~casto;
        inter = (t_sample)casto/1073741823.5 - 1.f;
        #ifdef FP_FAST_FMAF
        *out++ = fmaf(inter, mul, add);
        #else
        *out++ = inter*mul + add;
        #endif
    }
    return (w+5);
}

INTERN void shadylib_atoms_copy(int argc, t_atom *from, t_atom *to)
{
    int i;
    for (i = 0; i < argc; i++)
        to[i] = from[i];
}

t_class *shadylib_alist_class;

INTERN void shadylib_alist_init(t_shadylib_alist *x)
{
    x->l_pd = shadylib_alist_class;
    x->l_n = x->l_npointer = 0;
    x->l_vec = 0;
}

INTERN void shadylib_alist_clear(t_shadylib_alist *x)
{
    int i;
    for (i = 0; i < x->l_n; i++)
    {
        if (x->l_vec[i].l_a.a_type == A_POINTER)
            gpointer_unset(x->l_vec[i].l_a.a_w.w_gpointer);
    }
    if (x->l_vec)
        freebytes(x->l_vec, x->l_n * sizeof(*x->l_vec));
}

INTERN void shadylib_alist_copyin(t_shadylib_alist* x, t_symbol* SHADYLIB_UNUSED(s), int argc,
    t_atom *argv, int where)
{
    int i, j;
    for (i = 0, j = where; i < argc; i++, j++)
    {
        x->l_vec[j].l_a = argv[i];
        if (x->l_vec[j].l_a.a_type == A_POINTER)
        {
            x->l_npointer++;
            gpointer_copy(x->l_vec[j].l_a.a_w.w_gpointer, &x->l_vec[j].l_p);
            x->l_vec[j].l_a.a_w.w_gpointer = &x->l_vec[j].l_p;
        }
    }
}

    /* set contents to a list */
INTERN void shadylib_alist_list(t_shadylib_alist *x, t_symbol *s, int argc, t_atom *argv)
{
    shadylib_alist_clear(x);
    if (!(x->l_vec = (t_shadylib_listelem *)getbytes(argc * sizeof(*x->l_vec))))
    {
        x->l_n = 0;
        pd_error(0, "shadylib_alist: out of memory");
        return;
    }
    x->l_n = argc;
    x->l_npointer = 0;
    shadylib_alist_copyin(x, s, argc, argv, 0);
}

    /* set contents to an arbitrary non-list message */
INTERN void shadylib_alist_anything(t_shadylib_alist *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    shadylib_alist_clear(x);
    if (!(x->l_vec = (t_shadylib_listelem *)getbytes((argc+1) * sizeof(*x->l_vec))))
    {
        x->l_n = 0;
        pd_error(0, "shadylib_list_alloc: out of memory");
        return;
    }
    x->l_n = argc+1;
    x->l_npointer = 0;
    SETSYMBOL(&x->l_vec[0].l_a, s);
    for (i = 0; i < argc; i++)
    {
        x->l_vec[i+1].l_a = argv[i];
        if (x->l_vec[i+1].l_a.a_type == A_POINTER)
        {
            x->l_npointer++;
            gpointer_copy(x->l_vec[i+1].l_a.a_w.w_gpointer, &x->l_vec[i+1].l_p);
            x->l_vec[i+1].l_a.a_w.w_gpointer = &x->l_vec[i+1].l_p;
        }
    }
}

INTERN void shadylib_alist_toatoms(t_shadylib_alist *x, t_atom *to, int onset, int count)
{
    int i;
    for (i = 0; i < count; i++)
        to[i] = x->l_vec[onset + i].l_a;
}

INTERN void shadylib_alist_clone(t_shadylib_alist *x, t_shadylib_alist *y, int onset, int count)
{
    int i;
    y->l_pd = shadylib_alist_class;
    y->l_n = count;
    y->l_npointer = 0;
    if (!(y->l_vec = (t_shadylib_listelem *)getbytes(y->l_n * sizeof(*y->l_vec))))
    {
        y->l_n = 0;
        pd_error(0, "shadylib_list_alloc: out of memory");
    }
    else for (i = 0; i < count; i++)
    {
        y->l_vec[i].l_a = x->l_vec[onset + i].l_a;
        if (y->l_vec[i].l_a.a_type == A_POINTER)
        {
            gpointer_copy(y->l_vec[i].l_a.a_w.w_gpointer, &y->l_vec[i].l_p);
            y->l_vec[i].l_a.a_w.w_gpointer = &y->l_vec[i].l_p;
            y->l_npointer++;
        }
    }
}

INTERN void shadylib_alist_setup(void)
{
    shadylib_alist_class = class_new(gensym("list inlet"),
        0, 0, sizeof(t_shadylib_alist), 0, 0);
    class_addlist(shadylib_alist_class, shadylib_alist_list);
    class_addanything(shadylib_alist_class, shadylib_alist_anything);
}

static const char* print_atomtype (t_atomtype intype) {
    switch(intype) {
        case A_NULL: return "A_NULL";
        case A_FLOAT: return "A_FLOAT";
        case A_SYMBOL: return "A_SYMBOL";
        case A_POINTER: return "A_POINTER";
        case A_SEMI: return "A_SEMI";
        case A_COMMA: return "A_COMMA";
        case A_DEFFLOAT: return "A_DEFFLOAT";
        case A_DEFSYM: return "A_DEFSYM";
        case A_DOLLAR: return "A_DOLLAR";
        case A_DOLLSYM: return "A_DOLLSYM";
        case A_GIMME: return "A_GIMME";
        case A_CANT: return "A_CANT";
    }
    pd_error(0, "shadylib print_atomtype: no such atomtype");
    return "";
}

INTERN int shadylib_atoms_eq(t_atom *first, t_atom *second) {
    if (first->a_type == second->a_type) {
        switch(first->a_type) {
            case A_FLOAT:
                return first->a_w.w_float == second->a_w.w_float;
            case A_SYMBOL:
                return first->a_w.w_symbol == second->a_w.w_symbol;
            case A_POINTER:
                return first->a_w.w_gpointer == second->a_w.w_gpointer;
            default:
                pd_error(0, "shadylib_atms_cmp: unsupported type %s", print_atomtype(first->a_type));
                return 0;
        }
    }
    return 0;
}
