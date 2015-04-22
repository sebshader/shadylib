#include <math.h>
#include "m_pd.h"

typedef struct fmod
{
    t_object x_obj;
    t_float x_f;
    float x_g;
} t_fmod;

t_class *sigfmod_class;

static void *sigfmod_new(t_floatarg mod)
{
    t_fmod *x = (t_fmod *)pd_new(sigfmod_class);
    floatinlet_new(&x->x_obj, &x->x_g);
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_f = 0;
    if(mod == 0) mod = 1;
    x->x_g = mod;
    return (x);
}

static t_int *sigfmod_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    float f = *(float *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    //if(f == 0) f = 1;
    while (n--)
    {   
        *out++ = fmodf(*in++, f);
    }
    return (w + 5);
}

static void sigfmod_dsp(t_fmod *x, t_signal **sp)
{
    dsp_add(sigfmod_perform, 4, sp[0]->s_vec, &x->x_g,
            sp[1]->s_vec, sp[0]->s_n);
}

void fmod_tilde_setup(void)
{
    sigfmod_class = class_new(gensym("fmod~"), (t_newmethod)sigfmod_new, 0,
        sizeof(t_fmod), 0, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(sigfmod_class, t_fmod, x_f);
    class_addmethod(sigfmod_class, (t_method)sigfmod_dsp, gensym("dsp"), A_CANT, 0);
}