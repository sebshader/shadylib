#include "m_pd.h"
extern int ugen_getsortno(void);

#define DEFDELVS 64             /* LATER get this from canvas at DSP time */
static int delread_zero = 0;    /* four bytes of zero for delread~, vd~ */

/* ----------------------------- delwritec~ ----------------------------- */
t_class *sigdelwritec_class;

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

#define XTRASAMPS 4
#define SAMPBLK 4

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

static void *sigdelwritec_new(t_symbol *s, t_floatarg msec)
{
    t_sigdelwritec *x = (t_sigdelwritec *)pd_new(sigdelwritec_class);
    if (!*s->s_name) s = gensym("delwritec~");
    pd_bind(&x->x_obj.ob_pd, s);
    x->x_sym = s;
    x->x_deltime = msec;
    x->x_cspace.c_n = 0;
    x->x_cspace.c_vec = getbytes(XTRASAMPS * sizeof(t_sample));
    x->x_sortno = 0;
    x->x_vecsize = 0;
    x->x_f = 0;
    return (x);
}

static void sigdelwritec_clear(t_sigdelwritec *x) {
	t_sample *dex = x->x_cspace.c_vec;
	for(int i = 0; i < (x->x_cspace.c_n + XTRASAMPS); i++) dex[i] = 0;
}
static t_int *sigdelwritec_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_delwritectl *c = (t_delwritectl *)(w[2]);
    int n = (int)(w[3]);
    int phase = c->c_phase, nsamps = c->c_n;
    t_sample *vp = c->c_vec, *bp = vp + phase, *ep = vp + (c->c_n + XTRASAMPS);
    phase += n;

    while (n--)
    {
        t_sample f = *in++;
        if (PD_BIGORSMALL(f))
            f = 0;
        *bp++ = f;
        if (bp == ep)
        {
            vp[0] = ep[-4];
            vp[1] = ep[-3];
            vp[2] = ep[-2];
            vp[3] = ep[-1];
            bp = vp + XTRASAMPS;
            phase -= nsamps;
        }
    }
    c->c_phase = phase; 
    return (w+4);
}

static void sigdelwritec_dsp(t_sigdelwritec *x, t_signal **sp)
{
    dsp_add(sigdelwritec_perform, 3, sp[0]->s_vec, &x->x_cspace, sp[0]->s_n);
    x->x_sortno = ugen_getsortno();
    sigdelwritec_checkvecsize(x, sp[0]->s_n);
    sigdelwritec_updatesr(x, sp[0]->s_sr);
}

static void sigdelwritec_free(t_sigdelwritec *x)
{
    pd_unbind(&x->x_obj.ob_pd, x->x_sym);
    freebytes(x->x_cspace.c_vec,
        (x->x_cspace.c_n + XTRASAMPS) * sizeof(t_sample));
}

void delwritec_tilde_setup(void)
{
    sigdelwritec_class = class_new(gensym("delwritec~"), 
        (t_newmethod)sigdelwritec_new, (t_method)sigdelwritec_free,
        sizeof(t_sigdelwritec), 0, A_DEFSYM, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(sigdelwritec_class, t_sigdelwritec, x_f);
    class_addmethod(sigdelwritec_class, (t_method)sigdelwritec_dsp,
        gensym("dsp"), 0);
    class_addmethod(sigdelwritec_class, (t_method)sigdelwritec_clear,
    	gensym("clear"), 0);
}