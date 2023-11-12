#include "shadylib.h"

typedef struct modf
{
    t_object x_obj;
    t_float x_f;
} t_modf;

t_class *sigmodf_class;

static void *sigmodf_new() {
    t_modf *x = (t_modf *)pd_new(sigmodf_class);
    outlet_new(&x->x_obj, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static t_int *sigmodf_perform(t_int *w) {
    t_sample *in = (t_sample *)(w[1]);
    t_sample *out1 = (t_sample *)(w[2]);
    t_sample *out2 = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    float fint;
    while (n--) {
        *out2++ = modff(*in++, &fint);
        *out1++ = fint;
    }
    return (w + 5);
}

static void sigmodf_dsp(t_modf* UNUSED(x), t_signal **sp)
{
    dsp_add(sigmodf_perform, 4, sp[0]->s_vec, sp[1]->s_vec,
            sp[2]->s_vec, sp[0]->s_n);
}

void modf_tilde_setup(void)
{
    sigmodf_class = class_new(gensym("modf~"), (t_newmethod)sigmodf_new, 0,
        sizeof(t_modf), 0, 0);
    CLASS_MAINSIGNALIN(sigmodf_class, t_modf, x_f);
    class_addmethod(sigmodf_class, (t_method)sigmodf_dsp, gensym("dsp"), A_CANT, 0);
}