#include "shadylib.h"

static t_class *pib_tilde_class;

typedef struct _pib {
    t_object x_obj;
    double x_time;
} t_pib;

static t_int *pib_tilde_perform(t_int *w) {
    double *x = (double *)(w[1]);
    *x = clock_getlogicaltime();
    return (w + 2);
}

static void pib_tilde_bang(t_pib *x) {
    outlet_float(x->x_obj.ob_outlet, clock_gettimesince(x->x_time));
}

static void pib_tilde_dsp(t_pib *x, t_signal** SHADYLIB_UNUSED(sp))
{
    dsp_add(pib_tilde_perform, 1, &x->x_time);
}

static void *pib_tilde_new(void) {
    t_pib *x = (t_pib *)pd_new(pib_tilde_class);
    x->x_time = clock_getlogicaltime();
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

void pib_tilde_setup(void) {
    pib_tilde_class = class_new(gensym("pib~"), (t_newmethod)pib_tilde_new, 0,
        sizeof(t_pib), 0, 0);
    class_addmethod(pib_tilde_class, (t_method)pib_tilde_dsp, gensym("dsp"),
        A_CANT, 0);
    class_addbang(pib_tilde_class, pib_tilde_bang);
}
