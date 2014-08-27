#include "m_pd.h"
#include <math.h>

static t_class *transpose_tilde_class;

typedef struct _transpose_tilde
{
    t_object x_obj;
    t_float x_f;
    float x_lastin;//reminder to make this work for pd-double
    float x_lastout;
} t_transpose_tilde;

static void *transpose_tilde_new(void)
{
    t_transpose_tilde *x = (t_transpose_tilde *)pd_new(transpose_tilde_class);
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
    x->x_lastin = 0;
    x->x_lastout = 1;
    return (x);
}

t_int *transpose_tilde_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    t_transpose_tilde *x = (t_transpose_tilde *)(w[3]);
    int n = (int)(w[4]);
    float f = x->x_lastout, tin = x->x_lastin;
    while (n--)
    {
        if(tin != *in) {
        	tin = *in;
			f = exp2f(tin/12);
		}
		in++;
        *out++ = f;
    }
    x->x_lastout = f;
    x->x_lastin = tin;
    return (w+5);
}

static void transpose_tilde_dsp(t_transpose_tilde *x, t_signal **sp)
{
    dsp_add(transpose_tilde_perform, 4,
        sp[0]->s_vec, sp[1]->s_vec, x, sp[0]->s_n);
}

void transpose_tilde_setup(void)
{
    transpose_tilde_class = class_new(gensym("transpose~"), (t_newmethod)transpose_tilde_new, 0,
        sizeof(t_transpose_tilde), 0, 0);
    CLASS_MAINSIGNALIN(transpose_tilde_class, t_transpose_tilde, x_f);
    class_addmethod(transpose_tilde_class, (t_method)transpose_tilde_dsp, gensym("dsp"), A_CANT, 0);
}