#include "shadylib.h"

static t_class *transpose_tilde_class;

typedef struct _transpose_tilde
{
    t_object x_obj;
    t_float x_f;
} t_transpose_tilde;

static void *transpose_tilde_new(void)
{
    t_transpose_tilde *x = (t_transpose_tilde *)pd_new(transpose_tilde_class);
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
    return (x);
}

t_int *transpose_tilde_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out = (t_sample *)(w[2]);
    int n = (int)(w[3]);
    while (n--) *out++ = exp2f(*in++*(1.0/12));
    return (w+4);
}

static void transpose_tilde_dsp(t_transpose_tilde* UNUSED(x), t_signal **sp) {
    dsp_add(transpose_tilde_perform, 3,
        sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

void transpose_tilde_setup(void)
{
    transpose_tilde_class = class_new(gensym("transpose~"), (t_newmethod)transpose_tilde_new, 0,
        sizeof(t_transpose_tilde), 0, 0);
    CLASS_MAINSIGNALIN(transpose_tilde_class, t_transpose_tilde, x_f);
    class_addmethod(transpose_tilde_class, (t_method)transpose_tilde_dsp, gensym("dsp"), A_CANT, 0);
}