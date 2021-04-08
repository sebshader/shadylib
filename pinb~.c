#include "shadylib.h"

typedef struct pinb
{
    t_object x_obj;
} t_pinb;

t_class *sigpinb_class;

static void *sigpinb_new() {
    t_pinb *x = (t_pinb *)pd_new(sigpinb_class);
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static t_int *sigpinb_perform(t_int *w) {
	t_sample *out = (t_sample *)(w[1]);
    int n = (int)(w[2]);
    for(int i = 0; i < n; i++) {
    	*out++ = i;
    }
    return (w + 3);
}

static void sigpinb_dsp(t_pinb* UNUSED(x), t_signal **sp)
{
    dsp_add(sigpinb_perform, 2, sp[0]->s_vec, sp[0]->s_n);
}

void pinb_tilde_setup(void)
{
    sigpinb_class = class_new(gensym("pinb~"), (t_newmethod)sigpinb_new, 0,
        sizeof(t_pinb), CLASS_NOINLET, 0);
    class_addmethod(sigpinb_class, (t_method)sigpinb_dsp, gensym("dsp"), A_CANT, 0);
}