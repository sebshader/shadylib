#include "shadylib.h"

t_float *shadylib_gaustab; /* e^(-x^2) */
t_float *shadylib_couchtab; /* 1/(x^2 + 1) */
t_float *shadylib_rexptab; /* (e^(-x) - .001)/.999 to x=ln(.001) */
t_sample *shadylib_sintbl;
t_sample *shadylib_cosectbl;
unsigned char aligned = 0; //doesn't have to be fast

t_float shadylib_readtab(shadylib_t_tabtype type, t_float index) {
	t_float *tab = shadylib_rexptab + (type * SHABLESIZE);
	int iindex;
	t_float frac, index2;
	index *= SHABLESIZE;
	index = fmax(index, 0);
	if (index >= SHABLESIZE - 1) return tab[SHABLESIZE - 1];
	else {
		iindex = index;
		frac = index - iindex;
		index = tab[iindex++];
		index2 = tab[iindex] - index;
#ifdef FP_FAST_FMA
		return fma(frac, index2, index);
#else
        return index2*frac + index;
#endif
	}
}

void shadylib_maketabs(void) {
	double lnths;
	t_float incr = 4.f/(SHABLESIZE - 1);
	t_float sqr;
	if (shadylib_rexptab) return;
	shadylib_rexptab = getbytes(sizeof(t_float)*SHABLESIZE*3);
	shadylib_gaustab = shadylib_rexptab + SHABLESIZE;
	shadylib_couchtab = shadylib_gaustab + SHABLESIZE;
	for(int i = 0; i < SHABLESIZE; i++) {
		lnths = (i/(double)(SHABLESIZE - 1))*log(.001);
		shadylib_rexptab[i] = (exp(lnths) - .001)/.999;
		sqr = incr*i;
		shadylib_gaustab[i] = exp(-(sqr*sqr));
		sqr = sqr*8;
		sqr *= sqr;
		shadylib_couchtab[i] = 1/(sqr + 1);
	}
	/* fix shadylib_rexptab */
	shadylib_rexptab[SHABLESIZE - 1] = 0;
}

void shadylib_freetabs(t_class *dummy) {
	if(shadylib_rexptab) {
		freebytes(shadylib_rexptab, sizeof(t_float)*SHABLESIZE*3);
		shadylib_rexptab = NULL;
	}
}

/* create sine and cosecant tables */
void shadylib_makebuzz(void) {
	double phase;
	int i;
	
	if(shadylib_sintbl) return;
	
	shadylib_sintbl = (t_sample *)getbytes(sizeof(t_sample) * (BUZZSIZE + 1)*2);
	shadylib_cosectbl = shadylib_sintbl + BUZZSIZE + 1;
	double incr = 2*M_PI/BUZZSIZE;
	for(i = 0; i <= BUZZSIZE; i++) {
		phase = i*incr;
		shadylib_sintbl[i] = sin(phase);
		shadylib_cosectbl[i] = 1/shadylib_sintbl[i];
	}
	/* insert BADVALs */
	shadylib_cosectbl[0] = shadylib_cosectbl[BUZZSIZE/2] = shadylib_cosectbl[BUZZSIZE] = BADVAL;
	for (i=1; i<=8; ++i) {
		shadylib_cosectbl[i] = shadylib_cosectbl[BUZZSIZE-i] = BADVAL;
		shadylib_cosectbl[BUZZSIZE/2-i] = shadylib_cosectbl[BUZZSIZE/2+i] = BADVAL;
	}
}

void shadylib_freebuzz(t_class *dummy) {
	if(shadylib_sintbl) {
		freebytes(shadylib_sintbl, sizeof(sizeof(t_sample) * (BUZZSIZE + 1)*2));
		shadylib_sintbl = NULL;
	}
}

t_float shadylib_scalerange (t_float a) {
	return (a - ENVELOPE_RANGE)/ENVELOPE_MAX;
}

t_float shadylib_ain2reala(t_float a) {
	return exp2(log2(ENVELOPE_RANGE)*(1 - a));
}

t_int shadylib_ms2samps(t_float time, t_float sr)
{
	t_int samp = (t_int)(sr*time/1000.0);
	if(samp < 1) samp = 1;
	return samp;
}

void shadylib_f2axfade (t_float a, shadylib_t_stage *stage, int samesamp) {
	a = fmax(fmin(a, 1.0), 0.0);
	if(a != 1.0) { /*exponential*/
		a = shadylib_ain2reala(a);
		if(stage->lin == a && samesamp) return;
		stage->lin = a;
		stage->op = exp2(log2(a)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - a);
	} else { /*linear*/
		stage->lin = 1.0;
		stage->op = 1.0;
		stage->base = 1.0/stage->nsamp;
	}
}

void shadylib_ms2axfade (shadylib_t_stage *stage) {
	if (stage->lin != 1.0) {
		stage->op = exp2(log2(stage->lin)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - stage->lin);
	} else {
		stage->base = 1.0/stage->nsamp;
		stage->op = 1.0; //linear
	}
}

void shadylib_f2dxfade(t_float a, shadylib_t_stage *stage, int samesamp) {
	a = fmax(fmin(a, 1.0), 0.0);
	if(a != 1.0) {/*exponential*/
		a = shadylib_ain2reala(a);
		if(stage->lin == shadylib_scalerange(a) && samesamp) return;
		stage->lin = shadylib_scalerange(a);
		stage->op = exp2(log2(a)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - stage->lin);
	} else {/*linear*/
		stage->lin = 1.0;
		stage->base = 1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

void shadylib_ms2dxfade (shadylib_t_stage *stage) {
	if(stage->lin != 1.0) {
		stage->op = exp2(log2(stage->lin*ENVELOPE_MAX + ENVELOPE_RANGE)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - stage->lin);
	} else {
		stage->base = 1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

void shadylib_f2rxfade(t_float a, shadylib_t_stage *stage, int samesamp) {
	a = fmax(fmin(a, 1.0), 0.0);
	if(a != 1.0) {/*exponential*/
		a = shadylib_ain2reala(a);
		if(stage->lin == shadylib_scalerange(a) && samesamp) return;
		stage->lin = shadylib_scalerange(a);
		stage->op = exp2(log2(a)/stage->nsamp);
		stage->base = stage->lin*(stage->op - 1)/(1 - stage->lin);
	} else {/*linear*/
		stage->lin = 1.0;
		stage->base = -1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

void shadylib_ms2rxfade (shadylib_t_stage *stage) {
	if(stage->lin != 1.0) {
		stage->op = exp2(log2(stage->lin*ENVELOPE_MAX + ENVELOPE_RANGE)/stage->nsamp);
		stage->base = stage->lin*(stage->op - 1)/(1 - stage->lin);
	} else {
		stage->base = -1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

/* needed for delay objects */
t_class *sigdelwritec_class;

void sigdelwritec_updatesr (t_sigdelwritec *x, t_float sr) /* added by Mathieu Bouchard */
{
    int nsamps = x->x_deltime * sr * (t_float)(0.001f);
    if (nsamps < 1) nsamps = 1;
    nsamps += ((- nsamps) & (SAMPBLK - 1));
    nsamps += DEFDELVS;
    if (x->x_cspace.c_n != nsamps) {
      x->x_cspace.c_vec = (t_sample *)resizebytes(x->x_cspace.c_vec,
        (x->x_cspace.c_n + XTRASAMPS) * sizeof(t_sample),
        (         nsamps + XTRASAMPS) * sizeof(t_sample));
      x->x_cspace.c_n = nsamps;
      x->x_cspace.c_phase = XTRASAMPS;
    }
}

    /* routine to check that all delwrites/delreads/vds have same vecsize */
void sigdelwritec_checkvecsize(t_sigdelwritec *x, int vecsize)
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

void shadylib_checkalign(void) {
	union shadylib_tabfudge tf;
	if(aligned) return;
	tf.tf_d = UNITBIT32 + 0.5;
    if ((unsigned)tf.tf_i[LOWOFFSET] != 0x80000000)
        bug("shadylib: unexpected machine alignment");
    else aligned = 1;
}

t_int *shadylib_opd_perf0(t_int *w) {
	shadylib_t_oscctl *x = (shadylib_t_oscctl *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_sample *add = x->invals[1].vec;
	t_sample *tab = shadylib_sintbl, *addr, f1, f2, frac;
	
    double dphase;
    int normhipart;
    union shadylib_tabfudge tf;
    
    tf.tf_d = UNITBIT32;
    normhipart = tf.tf_i[HIOFFSET];


        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
        tf.tf_d = dphase;
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
        tf.tf_i[HIOFFSET] = normhipart;
    while (--n)
    {
        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
            frac = tf.tf_d - UNITBIT32;
        tf.tf_d = dphase;
            f1 = addr[0];
            f2 = addr[1];
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
        	#ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, (*mul++), (*add++));
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + (*add++);
            #endif
        tf.tf_i[HIOFFSET] = normhipart;
    }
            frac = tf.tf_d - UNITBIT32;
            f1 = addr[0];
            f2 = addr[1];
            #ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, (*mul++), (*add++));
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + (*add++);
            #endif
    return (w+5);
}

t_int *shadylib_opd_perf1(t_int *w) {
	shadylib_t_oscctl *x = (shadylib_t_oscctl *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_float add = x->invals[1].val;
	t_sample *tab = shadylib_sintbl, *addr, f1, f2, frac;
	
    double dphase;
    int normhipart;
    union shadylib_tabfudge tf;
    
    tf.tf_d = UNITBIT32;
    normhipart = tf.tf_i[HIOFFSET];


        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
        tf.tf_d = dphase;
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
        tf.tf_i[HIOFFSET] = normhipart;
    while (--n)
    {
        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
            frac = tf.tf_d - UNITBIT32;
        tf.tf_d = dphase;
            f1 = addr[0];
            f2 = addr[1];
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
        #ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, (*mul++), add);
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + add;
            #endif
        tf.tf_i[HIOFFSET] = normhipart;
    }
            frac = tf.tf_d - UNITBIT32;
            f1 = addr[0];
            f2 = addr[1];
            #ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, (*mul++), add);
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + add;
            #endif
    return (w+5);
}

t_int *shadylib_opd_perf2(t_int *w) {
	shadylib_t_oscctl *x = (shadylib_t_oscctl *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_float mul = x->invals[0].val;
	t_float add = x->invals[1].val;
	t_sample *tab = shadylib_sintbl, *addr, f1, f2, frac;
	
    double dphase;
    int normhipart;
    union shadylib_tabfudge tf;
    
    tf.tf_d = UNITBIT32;
    normhipart = tf.tf_i[HIOFFSET];


        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
        tf.tf_d = dphase;
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
        tf.tf_i[HIOFFSET] = normhipart;
    while (--n)
    {
        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
            frac = tf.tf_d - UNITBIT32;
        tf.tf_d = dphase;
            f1 = addr[0];
            f2 = addr[1];
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
    		#ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, mul, add);
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*mul + add;
            #endif
        tf.tf_i[HIOFFSET] = normhipart;
    }
            frac = tf.tf_d - UNITBIT32;
            f1 = addr[0];
            f2 = addr[1];
            #ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, mul, add);
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*mul + add;
            #endif
    return (w+5);
}

t_int *shadylib_recd_perf0(t_int *w) {
	shadylib_t_oscctl *x = (shadylib_t_oscctl *)(w[1]);
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
        casto = (uint32_t)(*in++ * 4294967295);
        /* set the sign bit of double 1.0 */
        inter.tf_i[HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        #ifdef FP_FAST_FMA
        *out++ = fma(inter.tf_d, *mul++, *add++);
        #else
        *out++ = inter.tf_d*(*mul++) + (*add++);
        #endif
    }
    return (w+5);
}

t_int *shadylib_recd_perf1(t_int *w) {
	shadylib_t_oscctl *x = (shadylib_t_oscctl *)(w[1]);
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
        casto = (uint32_t)(*in++ * 4294967295);
        inter.tf_i[HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        #ifdef FP_FAST_FMA
        *out++ = fma(inter.tf_d, *mul++, add);
        #else
        *out++ = inter.tf_d*(*mul++) + add;
        #endif
    }
    return (w+5);
}

t_int *shadylib_recd_perf2(t_int *w) {
	shadylib_t_oscctl *x = (shadylib_t_oscctl *)(w[1]);
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
        casto = (uint32_t)(*in++ * 4294967295);
        inter.tf_i[HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        #ifdef FP_FAST_FMA
        *out++ = fma(inter.tf_d, mul, add);
        #else
        *out++ = inter.tf_d*mul + add;
        #endif
    }
    return (w+5);
}

t_int *shadylib_trid_perf0(t_int *w) {
	shadylib_t_oscctl *x = (shadylib_t_oscctl *)(w[1]);
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
        inter = (t_sample)casto/1073741823.5 - 1;
        #ifdef FP_FAST_FMA
        *out++ = fma(inter, *mul++, *add++);
        #else
        *out++ = inter*(*mul++) + (*add++);
        #endif
    }
    return (w+5);
}

t_int *shadylib_trid_perf1(t_int *w) {
	shadylib_t_oscctl *x = (shadylib_t_oscctl *)(w[1]);
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
        inter = (t_sample)casto/1073741823.5 - 1;
        #ifdef FP_FAST_FMA
        *out++ = fma(inter, *mul++, add);
        #else
        *out++ = inter*(*mul++) + add;
        #endif
    }
    return (w+5);
}

t_int *shadylib_trid_perf2(t_int *w) {
	shadylib_t_oscctl *x = (shadylib_t_oscctl *)(w[1]);
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
        inter = (t_sample)casto/1073741823.5 - 1;
        #ifdef FP_FAST_FMA
        *out++ = fma(inter, mul, add);
        #else
        *out++ = inter*mul + add;
        #endif
    }
    return (w+5);
}

void atoms_copy(int argc, t_atom *from, t_atom *to)
{
    int i;
    for (i = 0; i < argc; i++)
        to[i] = from[i];
}

t_class *alist_class;

void alist_init(t_alist *x)
{
    x->l_pd = alist_class;
    x->l_n = x->l_npointer = 0;
    x->l_vec = 0;
}

void alist_clear(t_alist *x)
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

void alist_copyin(t_alist *x, t_symbol *s, int argc, t_atom *argv,
    int where)
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
void alist_list(t_alist *x, t_symbol *s, int argc, t_atom *argv)
{
    alist_clear(x);
    if (!(x->l_vec = (t_listelem *)getbytes(argc * sizeof(*x->l_vec))))
    {
        x->l_n = 0;
        error("list: out of memory");
        return;
    }
    x->l_n = argc;
    x->l_npointer = 0;
    alist_copyin(x, s, argc, argv, 0);
}

    /* set contents to an arbitrary non-list message */
void alist_anything(t_alist *x, t_symbol *s, int argc, t_atom *argv)
{
    int i;
    alist_clear(x);
    if (!(x->l_vec = (t_listelem *)getbytes((argc+1) * sizeof(*x->l_vec))))
    {
        x->l_n = 0;
        error("list_alloc: out of memory");
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

void alist_toatoms(t_alist *x, t_atom *to, int onset, int count)
{
    int i;
    for (i = 0; i < count; i++)
        to[i] = x->l_vec[onset + i].l_a;
}


void alist_clone(t_alist *x, t_alist *y, int onset, int count)
{
    int i;
    y->l_pd = alist_class;
    y->l_n = count;
    y->l_npointer = 0;
    if (!(y->l_vec = (t_listelem *)getbytes(y->l_n * sizeof(*y->l_vec))))
    {
        y->l_n = 0;
        error("list_alloc: out of memory");
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

void alist_setup(void)
{
    alist_class = class_new(gensym("list inlet"),
        0, 0, sizeof(t_alist), 0, 0);
    class_addlist(alist_class, alist_list);
    class_addanything(alist_class, alist_anything);
}
