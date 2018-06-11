/* Supercollider Code */

#include <math.h>
#include "m_pd.h"

static t_class *siglag_class;

typedef struct siglag
{
    t_object x_obj;
    t_float x_f;
    double x_last;
} t_siglag;

static void *siglag_new(t_float f)
{
    t_siglag *x = (t_siglag *)pd_new(siglag_class);
    pd_float(
        (t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal),
            f);
    outlet_new(&x->x_obj, &s_signal);
    x->x_last = 0;
    return (x);
}

static t_int *siglag_perform(t_int *w)
{
    t_sample *in1 = (t_sample *)(w[1]);
    t_sample *in2 = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    t_siglag *x = (t_siglag *)(w[4]);
    int n = (t_int)(w[5]);
    int i;
    double last = x->x_last;
    for (i = 0; i < n; i++)
    {
        t_sample next = *in1++;
        t_sample coef = *in2++;
        *out++ = last = next + coef*(last - next);
    }
    if (PD_BIGORSMALL(last))
        last = 0;
    x->x_last = last;
    return (w+6);
}

static void siglag_dsp(t_siglag *x, t_signal **sp)
{
    dsp_add(siglag_perform, 5,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,
            x, sp[0]->s_n);
}

static void siglag_set(t_siglag *x, t_float f)
{
    x->x_last = f;
}

void lag_tilde_setup(void)
{
    siglag_class = class_new(gensym("lag~"),
        (t_newmethod)siglag_new, 0, sizeof(t_siglag), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(siglag_class, t_siglag, x_f);
    class_addmethod(siglag_class, (t_method)siglag_set,
        gensym("set"), A_DEFFLOAT, 0);
    class_addmethod(siglag_class, (t_method)siglag_dsp,
        gensym("dsp"), A_CANT, 0);
}