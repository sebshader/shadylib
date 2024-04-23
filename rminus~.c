#include "m_pd.h"

static t_class *scalarrminus_class;

typedef struct _scalarrminus
{
    t_object x_obj;
    t_float x_f;
    t_float x_g;
} t_scalarrminus;

static void *rminus_new(t_floatarg g)
{
    t_scalarrminus *x = (t_scalarrminus *)pd_new(scalarrminus_class);
    floatinlet_new(&x->x_obj, &x->x_g);
    x->x_g = g;
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0.f;
    return (x);
}

static t_int *scalarrminus_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_float f = *(t_float *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    while (n--) *out++ = f - *in++;
    return (w+5);
}

static t_int *scalarrminus_perf8(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_float g = *(t_float *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    for (; n; n -= 8, in += 8, out += 8)
    {
        t_sample f0 = in[0], f1 = in[1], f2 = in[2], f3 = in[3];
        t_sample f4 = in[4], f5 = in[5], f6 = in[6], f7 = in[7];

        out[0] = g - f0; out[1] = g - f1; out[2] = g - f2; out[3] = g - f3;
        out[4] = g - f4; out[5] = g - f5; out[6] = g - f6; out[7] = g - f7;
    }
    return (w+5);
}

static void scalarrminus_dsp(t_scalarrminus *x, t_signal **sp)
{
    if (sp[0]->s_n&7)
        dsp_add(scalarrminus_perform, 4, sp[0]->s_vec, &x->x_g,
            sp[1]->s_vec, sp[0]->s_n);
    else
        dsp_add(scalarrminus_perf8, 4, sp[0]->s_vec, &x->x_g,
            sp[1]->s_vec, sp[0]->s_n);
}

void rminus_tilde_setup(void)
{
    scalarrminus_class = class_new(gensym("rminus~"), (t_newmethod)rminus_new, 0,
        sizeof(t_scalarrminus), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(scalarrminus_class, t_scalarrminus, x_f);
    class_addmethod(scalarrminus_class, (t_method)scalarrminus_dsp,
        gensym("dsp"), A_CANT, 0);
}
