/* real allpass filter */
#include "shadylib.h"

static t_class *realpass_class;
/*modified from pd source */

typedef struct _realpass
{
    t_object x_obj;
    t_float x_f;
    t_float x_sr;
    t_float x_coef;
    t_float x_ttime;  /* delay line length msec */
    int c_n; /* same in samples */
    int c_phase; /* current write point */
    t_sample *c_vec; /* ring buffer for delay line */
} t_realpass;

static void realpass_updatesr (t_realpass *x, t_float sr) /* added by Mathieu Bouchard */
{
    int nsamps = x->x_ttime * sr * (t_float)(0.001f);
    if (nsamps < 1) nsamps = 1;
    nsamps += ((- nsamps) & (SHADYLIB_SAMPBLK - 1));
    nsamps += SHADYLIB_DEFDELVS;
    if (x->c_n != nsamps) {
      x->c_vec = (t_sample *)resizebytes(x->c_vec,
        (x->c_n + SHADYLIB_XTRASAMPS) * sizeof(t_sample),
        (nsamps + SHADYLIB_XTRASAMPS) * sizeof(t_sample));
      x->c_n = nsamps;
      x->c_phase = SHADYLIB_XTRASAMPS;
    }
}

static t_int *realpass_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *time = (t_sample *)(w[2]);
    t_sample *coef = (t_sample *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    t_realpass *x = (t_realpass *)(w[5]);
    int n = (int)(w[6]);

    int phase = x->c_phase, nsamps = x->c_n;
    int idelsamps;
    t_sample limit = nsamps - 3;
    t_sample *vp = x->c_vec, *bp, *wp = vp + phase, 
    	*ep = vp + (nsamps + SHADYLIB_XTRASAMPS);
    t_sample delsamps, frac, a, b, c, d, cminusb;
    phase += n;
    while (n--)
    {
    	t_sample f = *in++;
        if (PD_BIGORSMALL(f))
            f = 0;

        delsamps = x->x_sr * (*time++);
        
        if (!(delsamps >= 1.f))    /* too small or NAN */
            delsamps = 1.f;
        if (delsamps > limit)           /* too big */
            delsamps = limit;
        idelsamps = delsamps;
        frac = delsamps - (t_sample)idelsamps;
        bp = wp - idelsamps;
        if (bp < vp + 4) bp += nsamps;
        d = bp[-3];
        c = bp[-2];
        b = bp[-1];
        a = bp[0];
        cminusb = c-b;
       #ifdef FP_FAST_FMA
       delsamps = fma(frac, (
            cminusb - 0.1666667f * (1.-frac) * fma(
                (fma(-3.0f, cminusb, d - a)), frac, fma(-3.0, b, fma(2.0, a, d))
            )
        ), b);
        #else
        delsamps = b + frac * (
            cminusb - 0.1666667f * (1.-frac) * (
                (d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
            )
        );
        #endif
        
        a = fmax(fmin(*coef++, 1), -1);
        #ifdef FP_FAST_FMA
        c = fma(delsamps, -a, f);
        #else
        c = f - delsamps*a;
        #endif
        *wp++ = c;        
        *out++ = c*a + delsamps;
        if (wp == ep)
        {
            vp[0] = ep[-4];
            vp[1] = ep[-3];
            vp[2] = ep[-2];
            vp[3] = ep[-1];
            wp = vp + SHADYLIB_XTRASAMPS;
            phase -= nsamps;
        }
    }
    x->c_phase = phase;
    return (w+7);
}

static void realpass_dsp(t_realpass *x, t_signal **sp)
{
    dsp_add(realpass_perform, 6, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
    	sp[3]->s_vec, x, sp[0]->s_n);
   	x->x_sr = sp[0]->s_sr * 0.001;
    realpass_updatesr(x, sp[0]->s_sr);
}

static void *realpass_new(t_float time, t_float coef, t_float size)
{
    t_realpass *x = (t_realpass *)pd_new(realpass_class);
    if(size <= 0.0) size = 1000;
    if(time < 0.0) time = 0.0;
    x->x_ttime = size;
    
    signalinlet_new(&x->x_obj, time);
    signalinlet_new(&x->x_obj, coef);

    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
    x->c_n = 0;
    x->c_vec = getbytes(SHADYLIB_XTRASAMPS * sizeof(t_sample));
    memset((char *)(x->c_vec), 0, 
		sizeof(t_sample)*(SHADYLIB_XTRASAMPS));
    return(x);
}

static void realpass_clear(t_realpass *x) {
	memset((char *)(x->c_vec), 0, 
		sizeof(t_sample)*(x->c_n + SHADYLIB_XTRASAMPS));
}

static void realpass_free(t_realpass *x)
{
    freebytes(x->c_vec,
        (x->c_n + SHADYLIB_XTRASAMPS) * sizeof(t_sample));
}

void realpass_tilde_setup(void)
{
    realpass_class = class_new(gensym("realpass~"), 
        (t_newmethod)realpass_new, (t_method)realpass_free,
        sizeof(t_realpass), 0, A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(realpass_class, t_realpass, x_f);
    class_addmethod(realpass_class, (t_method)realpass_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(realpass_class, (t_method)realpass_clear,
    	gensym("clear"), 0);
}