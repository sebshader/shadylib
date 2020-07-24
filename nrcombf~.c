/* non-recirculating comb filter */
#include "shadylib.h"
#include <string.h>

/*modified from pd source */

static t_class *nrcombf_class;

typedef struct _nrcombf
{
    t_object x_obj;
    t_float x_f;
    t_float x_sr;
    t_float x_ttime;  /* delay line length msec */
    int c_n; /* same in samples */
    int c_phase; /* current write point */
    int norm; /* whether to normalize or not */
    t_sample *c_vec; /* ring buffer for delay line */
} t_nrcombf;

static void nrcombf_updatesr (t_nrcombf *x, t_float sr) /* added by Mathieu Bouchard */
{
    int nsamps = x->x_ttime * sr * (t_float)(0.001f);
    if (nsamps < 1) nsamps = 1;
    nsamps += ((- nsamps) & (SAMPBLK - 1));
    nsamps += DEFDELVS;
    if (x->c_n != nsamps) {
      x->c_vec = (t_sample *)resizebytes(x->c_vec,
        (x->c_n + XTRASAMPS) * sizeof(t_sample),
        (nsamps + XTRASAMPS) * sizeof(t_sample));
      x->c_n = nsamps;
      x->c_phase = XTRASAMPS;
    }
}

static t_int *nrcombf_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *time = (t_sample *)(w[2]);
    t_sample *fb = (t_sample *)(w[3]);
    t_sample *norm = (t_sample *)(w[4]);
    t_sample *out = (t_sample *)(w[5]);
    t_nrcombf *x = (t_nrcombf *)(w[6]);
    int n = (int)(w[7]);

    int phase = x->c_phase, nsamps = x->c_n;
    int idelsamps;
    t_sample limit = nsamps - 3;
    t_sample *vp = x->c_vec, *bp, *wp = vp + phase, 
    	*ep = vp + (nsamps + XTRASAMPS);
    phase += n;
    while (n--)
    {
    	t_sample f = *in++;
        if (PD_BIGORSMALL(f))
            f = 0;
        t_sample delsamps = x->x_sr * (*time++), frac;
        
        t_sample a, b, c, d, cminusb;
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
        b = fmax(fmin(*norm++, 1), -1);
        a = fmax(fmin(*fb++, 0x1.fffffp-1), -0x1.fffffp-1);
        #ifdef FP_FAST_FMA
        *out++ = fma(delsamps, a, f)*b;
        #else
        *out++ = (f + delsamps*a)*b;
        #endif
        *wp++ = f;
        if (wp == ep)
        {
            vp[0] = ep[-4];
            vp[1] = ep[-3];
            vp[2] = ep[-2];
            vp[3] = ep[-1];
            wp = vp + XTRASAMPS;
            phase -= nsamps;
        }
    }
    x->c_phase = phase;
    return (w+8);
}

static t_int *nnrcombf_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *time = (t_sample *)(w[2]);
    t_sample *fb = (t_sample *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    t_nrcombf *x = (t_nrcombf *)(w[5]);
    int n = (int)(w[6]);

    int phase = x->c_phase, nsamps = x->c_n;
    int idelsamps;
    t_sample limit = nsamps - 3;
    t_sample *vp = x->c_vec, *bp, *wp = vp + phase, 
    	*ep = vp + (nsamps + XTRASAMPS);
    phase += n;
    while (n--)
    {
    	t_sample f = *in++;
        if (PD_BIGORSMALL(f))
            f = 0;
        t_sample delsamps = x->x_sr * (*time++), frac;
        
        t_sample a, b, c, d, cminusb;
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
        a = fmax(fmin(*fb++, 0x1.fffffp-1), -0x1.fffffp-1);
        #ifdef FP_FAST_FMA
        *out++ = fma(delsamps, a, f)/(1 + fabs(a));
        #else
        *out++ = (f + delsamps*a)/(1 + fabs(a));
        #endif
        *wp++ = f;
        if (wp == ep)
        {
            vp[0] = ep[-4];
            vp[1] = ep[-3];
            vp[2] = ep[-2];
            vp[3] = ep[-1];
            wp = vp + XTRASAMPS;
            phase -= nsamps;
        }
    }
    x->c_phase = phase;
    return (w+7);
}

static void nrcombf_dsp(t_nrcombf *x, t_signal **sp)
{
	if(x->norm)
		dsp_add(nnrcombf_perform, 6, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
    		sp[3]->s_vec, x, sp[0]->s_n);
    else dsp_add(nrcombf_perform, 7, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
    	sp[3]->s_vec, sp[4]->s_vec, x, sp[0]->s_n);
   	x->x_sr = sp[0]->s_sr * 0.001;
    nrcombf_updatesr(x, sp[0]->s_sr);
}

static void *nrcombf_new(t_symbol *s, int argc, t_atom *argv)
{
	t_float time = 1000;
	t_float size = 0.0;
	t_float fb = 0;
	t_symbol *sarg;
	int norm = 0, i = 0, which = 0;
    t_nrcombf *x = (t_nrcombf *)pd_new(nrcombf_class);
    for(; i < argc; i++)
    	if (argv[i].a_type == A_FLOAT) {
    		switch (which) {
    			case 0:
					time = atom_getfloatarg(i, argc, argv);
					if (time < 0.0) time = 0.0;
					break;
				case 1:
					fb = atom_getfloatarg(i, argc, argv);
					break;
				case 2:
					size = atom_getfloatarg(i, argc, argv);
				default:;
			}
			which++;
    	} else {
    		sarg = atom_getsymbolarg(i, argc, argv);
    		if (!strcmp(sarg->s_name, "-n")) norm = 1;
    		else if(!strcmp(sarg->s_name, "-l")) {
    			i++;
    			if(i < argc)
    				size = atom_getfloatarg(i, argc, argv);
    		}
		}
    signalinlet_new(&x->x_obj, time);
    signalinlet_new(&x->x_obj, fb);
    if(size <= 0.0) size = time;
	x->x_ttime = size;
	if(!norm)
		signalinlet_new(&x->x_obj, 1.0);
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
    x->c_n = 0;
    x->c_vec = getbytes(XTRASAMPS * sizeof(t_sample));
    memset((char *)(x->c_vec), 0, 
		sizeof(t_sample)*(XTRASAMPS));
    x->norm = norm;
    return(x);
}

static void nrcombf_clear(t_nrcombf *x) {
	memset((char *)(x->c_vec), 0, 
		sizeof(t_sample)*(x->c_n + XTRASAMPS));
}

static void nrcombf_free(t_nrcombf *x)
{
    freebytes(x->c_vec,
        (x->c_n + XTRASAMPS) * sizeof(t_sample));
}

void nrcombf_tilde_setup(void)
{
    nrcombf_class = class_new(gensym("nrcombf~"), 
        (t_newmethod)nrcombf_new, (t_method)nrcombf_free,
        sizeof(t_nrcombf), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(nrcombf_class, t_nrcombf, x_f);
    class_addmethod(nrcombf_class, (t_method)nrcombf_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(nrcombf_class, (t_method)nrcombf_clear,
    	gensym("clear"), 0);
}