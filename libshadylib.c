#include "shadylib.h"

t_float *gaustab; /* e^(-x^2) */
t_float *couchtab; /* 1/(x^2 + 1) */
t_float *rexptab; /* (e^(-x) - .001)/.999 to x=ln(.001) */
t_sample *sintbl;
t_sample *cosectbl;
unsigned char aligned = 0; //doesn't have to be fast

t_float readtab(t_tabtype type, t_float index) {
	t_float *tab = rexptab + (type * SHABLESIZE);
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

void maketabs(void) {
	double lnths;
	t_float incr = 4.f/(SHABLESIZE - 1);
	t_float sqr;
	if (rexptab) return;
	rexptab = getbytes(sizeof(t_float)*SHABLESIZE*3);
	gaustab = rexptab + SHABLESIZE;
	couchtab = gaustab + SHABLESIZE;
	for(int i = 0; i < SHABLESIZE; i++) {
		lnths = (i/(double)(SHABLESIZE - 1))*log(.001);
		rexptab[i] = (exp(lnths) - .001)/.999;
		sqr = incr*i;
		gaustab[i] = exp(-(sqr*sqr));
		sqr = sqr*8;
		sqr *= sqr;
		couchtab[i] = 1/(sqr + 1);
	}
	/* fix rexptab */
	rexptab[SHABLESIZE - 1] = 0;
}

/* create sine and cosecant tables */
void makebuzz(void) {
	double phase;
	int i;
	
	if(sintbl) return;
	
	sintbl = (t_sample *)getbytes(sizeof(t_sample) * (BUZZSIZE + 1)*2);
	cosectbl = sintbl + BUZZSIZE + 1;
	double incr = 2*M_PI/BUZZSIZE;
	for(i = 0; i <= BUZZSIZE; i++) {
		phase = i*incr;
		sintbl[i] = sin(phase);
		cosectbl[i] = 1/sintbl[i];
	}
	/* insert BADVALs */
	cosectbl[0] = cosectbl[BUZZSIZE/2] = cosectbl[BUZZSIZE] = BADVAL;
	for (i=1; i<=8; ++i) {
		cosectbl[i] = cosectbl[BUZZSIZE-i] = BADVAL;
		cosectbl[BUZZSIZE/2-i] = cosectbl[BUZZSIZE/2+i] = BADVAL;
	}
}


t_float scalerange (t_float a) {
	return (a - ENVELOPE_RANGE)/ENVELOPE_MAX;
}

t_float ain2reala(t_float a) {
	return exp2(log2(ENVELOPE_RANGE)*(1 - a));
}

t_int ms2samps(t_float time, t_float sr)
{
	t_int samp = (t_int)(sr*time/1000.0);
	if(samp < 1) samp = 1;
	return samp;
}

void f2axfade (t_float a, t_stage *stage, int samesamp) {
	if(a > 1.0) a = 1.0;
	else if(a < 0.0) a = 0.0;
	if(a != 1.0) { /*exponential*/
		a = ain2reala(a);
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

void ms2axfade (t_stage *stage) {
	if (stage->lin != 1.0) {
		stage->op = exp2(log2(stage->lin)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - stage->lin);
	} else {
		stage->base = 1.0/stage->nsamp;
		stage->op = 1.0; //linear
	}
}

void f2dxfade(t_float a, t_stage *stage, int samesamp) {
	if(a > 1.0) a = 1.0;
	else if(a < 0.0) a = 0.0;
	if(a != 1.0) {/*exponential*/
		a = ain2reala(a);
		if(stage->lin == scalerange(a) && samesamp) return;
		stage->lin = scalerange(a);
		stage->op = exp2(log2(a)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - stage->lin);
	} else {/*linear*/
		stage->lin = 1.0;
		stage->base = 1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

void ms2dxfade (t_stage *stage) {
	if(stage->lin != 1.0) {
		stage->op = exp2(log2(stage->lin*ENVELOPE_MAX + ENVELOPE_RANGE)/stage->nsamp);
		stage->base = (1 - stage->op)/(1 - stage->lin);
	} else {
		stage->base = 1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

void f2rxfade(t_float a, t_stage *stage, int samesamp) {
	if(a > 1.0) a = 1.0;
	else if(a < 0.0) a = 0.0;
	if(a != 1.0) {/*exponential*/
		a = ain2reala(a);
		if(stage->lin == scalerange(a) && samesamp) return;
		stage->lin = scalerange(a);
		stage->op = exp2(log2(a)/stage->nsamp);
		stage->base = stage->lin*(stage->op - 1)/(1 - stage->lin);
	} else {/*linear*/
		stage->lin = 1.0;
		stage->base = -1.0/stage->nsamp;
		stage->op = 1.0;
	}
}

void ms2rxfade (t_stage *stage) {
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

void checkalign(void) {
	union tabfudge tf;
	if(aligned) return;
	tf.tf_d = UNITBIT32 + 0.5;
    if ((unsigned)tf.tf_i[LOWOFFSET] != 0x80000000)
        bug("shadylib: unexpected machine alignment");
    else aligned = 1;
}

t_int *opd_perf0(t_int *w) {
	t_oscctl *x = (t_oscctl *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_sample *add = x->invals[1].vec;
	t_sample *tab = sintbl, *addr, f1, f2, frac;
	
    double dphase;
    int normhipart;
    union tabfudge tf;
    
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

t_int *opd_perf1(t_int *w) {
	t_oscctl *x = (t_oscctl *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_float add = x->invals[1].val;
	t_sample *tab = sintbl, *addr, f1, f2, frac;
	
    double dphase;
    int normhipart;
    union tabfudge tf;
    
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

t_int *opd_perf2(t_int *w) {
	t_oscctl *x = (t_oscctl *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_float mul = x->invals[0].val;
	t_float add = x->invals[1].val;
	t_sample *tab = sintbl, *addr, f1, f2, frac;
	
    double dphase;
    int normhipart;
    union tabfudge tf;
    
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

t_int *recd_perf0(t_int *w) {
	t_oscctl *x = (t_oscctl *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_sample *add = x->invals[1].vec;
    union tabfudge inter;
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

t_int *recd_perf1(t_int *w) {
	t_oscctl *x = (t_oscctl *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_float add = x->invals[1].val;
	union tabfudge inter;
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

t_int *recd_perf2(t_int *w) {
	t_oscctl *x = (t_oscctl *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_float mul = x->invals[0].val;
	t_float add = x->invals[1].val;
	union tabfudge inter;
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

t_int *trid_perf0(t_int *w) {
	t_oscctl *x = (t_oscctl *)(w[1]);
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

t_int *trid_perf1(t_int *w) {
	t_oscctl *x = (t_oscctl *)(w[1]);
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

t_int *trid_perf2(t_int *w) {
	t_oscctl *x = (t_oscctl *)(w[1]);
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