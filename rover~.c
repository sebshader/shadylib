#include "m_pd.h"

static t_class *scalarrover_class;

typedef struct _rover
{
    t_object x_obj;
    t_float x_f;
} t_rover;

typedef struct _scalarrover
{
    t_object x_obj;
    t_float x_f;
    t_float x_g;
} t_scalarrover;

static void *rover_new(t_floatarg g)
{
    t_scalarrover *x = (t_scalarrover *)pd_new(scalarrover_class);
    floatinlet_new(&x->x_obj, &x->x_g);
    x->x_g = g;
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
    return (x);
}

static t_int *scalarrover_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_float f = *(t_float *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample g;
    while (n--) {
        g = *in++;
        *out++ = (g ? f / g : 0);
    }
    return (w+5);
}

static void scalarrover_dsp(t_scalarrover *x, t_signal **sp)
{
        dsp_add(scalarrover_perform, 4, sp[0]->s_vec, &x->x_g,
            sp[1]->s_vec, sp[0]->s_n);
}

void rover_tilde_setup(void)
{
    scalarrover_class = class_new(gensym("rover~"), (t_newmethod)rover_new, 0,
        sizeof(t_rover), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(scalarrover_class, t_scalarrover, x_f);
    class_addmethod(scalarrover_class, (t_method)scalarrover_dsp,
        gensym("dsp"), A_CANT, 0);
}