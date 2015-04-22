#include "shadylib.h"

static t_class *sigvdhs_class;

typedef struct _sigvdhs
{
    t_object x_obj;
    t_symbol *x_sym;
    t_float x_sr;       /* samples per msec */
    int x_zerodel;      /* 0 or vecsize depending on read/write order */
    t_float x_f;
} t_sigvdhs;

static void *sigvdhs_new(t_symbol *s)
{
    t_sigvdhs *x = (t_sigvdhs *)pd_new(sigvdhs_class);
    if (!*s->s_name) s = gensym("vdhs~");
    x->x_sym = s;
    x->x_sr = 1;
    x->x_zerodel = 0;
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
    return (x);
}

static t_int *sigvdhs_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_delwritectl *ctl = (t_delwritectl *)(w[3]);
    t_sigvdhs *x = (t_sigvdhs *)(w[4]);
    int n = (int)(w[5]);

    int nsamps = ctl->c_n;
    t_sample limit = nsamps - n - 1;
    t_sample fn = n-1;
    t_sample *vp = ctl->c_vec, *bp, *wp = vp + ctl->c_phase;
    t_sample zerodel = x->x_zerodel;
    while (n--)
    {
        t_sample delsamps = x->x_sr * *in++ - zerodel, frac;
        int idelsamps;
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
		a2 = a - 2.5 * b + 2.f * c - 0.5f * d;
		a3 = 0.5f * (d - a) + 1.5f * (b - c);

		*out++ =  ((a3 * frac + a2) * frac + a1) * frac + b;
    }
    return (w+6);
}

static void sigvdhs_dsp(t_sigvdhs *x, t_signal **sp)
{
    t_sigdelwritec *delwriter =
        (t_sigdelwritec *)pd_findbyclass(x->x_sym, sigdelwritec_class);
    x->x_sr = sp[0]->s_sr * 0.001;
    if (delwriter)
    {
        sigdelwritec_checkvecsize(delwriter, sp[0]->s_n);
        x->x_zerodel = (delwriter->x_sortno == ugen_getsortno() ?
            0 : delwriter->x_vecsize);
        dsp_add(sigvdhs_perform, 5,
            sp[0]->s_vec, sp[1]->s_vec,
                &delwriter->x_cspace, x, sp[0]->s_n);
    }
    else if (*x->x_sym->s_name)
        error("vdhs~: %s: no such delwritec~",x->x_sym->s_name);
}

void vdhs_tilde_setup(void)
{
    sigvdhs_class = class_new(gensym("vdhs~"), (t_newmethod)sigvdhs_new, 0,
        sizeof(t_sigvdhs), 0, A_DEFSYM, 0);
    class_addmethod(sigvdhs_class, (t_method)sigvdhs_dsp, gensym("dsp"), 0);
    CLASS_MAINSIGNALIN(sigvdhs_class, t_sigvdhs, x_f);
}