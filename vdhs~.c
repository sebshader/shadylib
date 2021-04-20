#include "shadylib.h"

/*modified from pd source */

static t_class *sigvdhs_class;

typedef struct _sigvdhs
{
    t_object x_obj;
    t_symbol *x_sym;
    t_float x_sr;       /* samples per msec */
    t_float x_n;
    int x_zerodel;      /* 0 or vecsize depending on read/write order */
    t_shadylib_delwritectl *x_cspace; /* pointer to the current delwritec~ ctl*/
	t_float x_mode;			/* which kind of interpolation? */
    t_float x_f;
} t_sigvdhs;

static void *sigvdhs_new(t_symbol *s, t_floatarg mode)
{
    t_sigvdhs *x = (t_sigvdhs *)pd_new(sigvdhs_class);
	floatinlet_new(&x->x_obj, &x->x_mode);
	if (mode < 0) mode = 0;
	else if (mode > 2) mode = 2;
	x->x_mode = mode;
    x->x_sym = s;
    x->x_sr = 1;
    x->x_n = 1;
    x->x_zerodel = 0;
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
    return (x);
}

static void sigvdhs_set(t_sigvdhs *x, t_symbol *s) {
    t_shadylib_sigdelwritec *delwriter =
        (t_shadylib_sigdelwritec *)pd_findbyclass(s,
        shadylib_sigdelwritec_class);
    if (delwriter) {
        x->x_sym = s;
        if(pd_getdspstate()) {
            shadylib_sigdelwritec_checkvecsize(delwriter, x->x_n);
            x->x_zerodel = (delwriter->x_sortno == ugen_getsortno() ?
                0 : delwriter->x_vecsize);
            x->x_cspace = &delwriter->x_cspace;
        }
    } else if (*x->x_sym->s_name)
        error("vdhs~: %s: no such delwritec~", s->s_name);
}

static t_int *sigvdhs_perform_no(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_shadylib_delwritectl *ctl = *(t_shadylib_delwritectl **)(w[3]);
    t_sigvdhs *x = (t_sigvdhs *)(w[4]);
    int n = (int)(w[5]);

    int nsamps = ctl->c_n;
    int limit = nsamps - n - 1;
    t_sample *vp = ctl->c_vec, *bp, *wp = vp + ctl->c_phase;
    t_sample zerodel = x->x_zerodel;
    int idelsamps;
    while (n--)
    {
        idelsamps = x->x_sr * *in++ - zerodel;
        
        if (idelsamps < 1) idelsamps = 1;
        if (idelsamps > limit) idelsamps = limit;
        idelsamps += n;
        bp = wp - idelsamps;
        if (bp < vp + 1) bp += nsamps;
        *out++ = *bp;
    }
    return (w+6);
}

static t_int *sigvdhs_perform_lin(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_shadylib_delwritectl *ctl = *(t_shadylib_delwritectl **)(w[3]);
    t_sigvdhs *x = (t_sigvdhs *)(w[4]);
    int n = (int)(w[5]);

    int nsamps = ctl->c_n;
    t_sample limit = nsamps - n - 1;
    t_sample fn = n-1;
    t_sample *vp = ctl->c_vec, *bp, *wp = vp + ctl->c_phase;
    t_sample zerodel = x->x_zerodel;
    int idelsamps;
    while (n--)
    {
        t_sample delsamps = x->x_sr * *in++ - zerodel, frac;
        
        t_sample a, b;
        if (delsamps < 1.00001f) delsamps = 1.00001f;
        if (delsamps > limit) delsamps = limit;
        delsamps += fn;
        fn = fn - 1.0f;
        idelsamps = delsamps;
        frac = delsamps - (t_sample)idelsamps;
        bp = wp - idelsamps;
        if (bp < vp + 2) bp += nsamps;
        b = bp[-1];
        a = bp[0];
		#ifdef FP_FAST_FMAF
		*out++ =  fmaf(b - a, frac, a);
		#else
		*out++ = a + (b - a)*frac;
		#endif
    }
    return (w+6);
}

static t_int *sigvdhs_perform_hs(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_shadylib_delwritectl *ctl = *(t_shadylib_delwritectl **)(w[3]);
    t_sigvdhs *x = (t_sigvdhs *)(w[4]);
    int n = (int)(w[5]);

    int nsamps = ctl->c_n;
    t_sample limit = nsamps - n - 1;
    t_sample fn = n-1;
    t_sample *vp = ctl->c_vec, *bp, *wp = vp + ctl->c_phase;
    t_sample zerodel = x->x_zerodel;
    int idelsamps;
    while (n--)
    {
        t_sample delsamps = x->x_sr * *in++ - zerodel, frac;
        
        t_sample a, b, c, d;
        double a3,a1,a2;
        if (delsamps < 1.00001f) delsamps = 1.00001f;
        if (delsamps > limit) delsamps = limit;
        delsamps += fn;
        fn = fn - 1.0f;
        idelsamps = delsamps;
        frac = delsamps - (t_sample)idelsamps;
        bp = wp - idelsamps;
        if (bp < vp + 4) bp += nsamps;
        d = bp[-3];
        c = bp[-2];
        b = bp[-1];
        a = bp[0];
        // 4-point, 3rd-order Hermite (x-form)
		a1 = 0.5f * (c - a);
		#ifdef FP_FAST_FMAF
		a2 =  fmaf(2.f, c, fmaf(0.5f, d, fmaf(2.5, b, a)));
		a3 = fmaf(0.5f, (d - a), 1.5f * (b - c));
		*out++ =  fmaf(fmaf(fmaf(a3, frac, a2), frac, a1), frac, b);
		#else
		a2 = a - 2.5 * b + 2.f * c - 0.5f * d;
		a3 = 0.5f * (d - a) + 1.5f * (b - c);
		*out++ =  ((a3 * frac + a2) * frac + a1) * frac + b;
		#endif
    }
    return (w+6);
}

static void sigvdhs_dsp(t_sigvdhs *x, t_signal **sp)
{
    t_shadylib_sigdelwritec *delwriter =
        (t_shadylib_sigdelwritec *)pd_findbyclass(x->x_sym, shadylib_sigdelwritec_class);
    x->x_sr = sp[0]->s_sr * 0.001;
    x->x_n = sp[0]->s_n;
    if (delwriter)
    {
		t_perfroutine modefn;
		switch((int)x->x_mode) {
			case 2:
				modefn = sigvdhs_perform_no;
				break;
			case 1:
				modefn = sigvdhs_perform_lin;
				break;
			default:
				modefn = sigvdhs_perform_hs;
		}
		/* no updatesr() but pd doesn't either for vd~/delread4~ ?? */
        shadylib_sigdelwritec_checkvecsize(delwriter, sp[0]->s_n);
        x->x_zerodel = (delwriter->x_sortno == ugen_getsortno() ?
            0 : delwriter->x_vecsize);
        x->x_cspace = &delwriter->x_cspace;
        dsp_add(modefn, 5,
            sp[0]->s_vec, sp[1]->s_vec,
                &x->x_cspace, x, sp[0]->s_n);
    }
    else if (*x->x_sym->s_name)
        error("vdhs~: %s: no such delwritec~", x->x_sym->s_name);
}

void vdhs_tilde_setup(void)
{
    sigvdhs_class = class_new(gensym("vdhs~"), (t_newmethod)sigvdhs_new, 0,
        sizeof(t_sigvdhs), 0, A_DEFSYM, A_DEFFLOAT, 0);
    class_addmethod(sigvdhs_class, (t_method)sigvdhs_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(sigvdhs_class, (t_method)sigvdhs_set,
        gensym("set"), A_SYMBOL, 0);
    CLASS_MAINSIGNALIN(sigvdhs_class, t_sigvdhs, x_f);
}
