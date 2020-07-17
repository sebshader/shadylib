#include "m_pd.h"

static t_class *scaler_class;

typedef struct _scaler {
	t_object x_obj;
	t_float x_f;
	t_float x_mul;
	t_float x_add;
} t_scaler;
	
t_int *scaler_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_float mul = *(t_float *)(w[2]);
    t_float add = *(t_float *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    int n = (int)(w[5]);
    while (n--)
    	#ifdef FP_FAST_FMA
    	*out++ = fma(*in++, mul, add);
    	#else
    	*out++ = (*in++)*mul + add;
    	#endif
    return (w+6);
}

t_int *scaler_perf8(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_float mul = *(t_float *)(w[2]);
    t_float add = *(t_float *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    int n = (int)(w[5]);
    for (; n; n -= 8, in += 8, out += 8)
    {
        out[0] = in[0]*mul + add;
        out[1] = in[1]*mul + add;
        out[2] = in[2]*mul + add;
        out[3] = in[3]*mul + add;
        out[4] = in[4]*mul + add;
        out[5] = in[5]*mul + add;
        out[6] = in[6]*mul + add;
        out[7] = in[7]*mul + add;
    }
    return (w+6);
}

static void scaler_dsp(t_scaler *x, t_signal **sp)
{
	if(sp[0]->s_n & 7)
    	dsp_add(scaler_perform, 5, sp[0]->s_vec, &x->x_mul,
            &x->x_add, sp[1]->s_vec, sp[0]->s_n);
    else
    	dsp_add(scaler_perf8, 5, sp[0]->s_vec, &x->x_mul,
            &x->x_add, sp[1]->s_vec, sp[0]->s_n);
}
	
static void *scaler_new(t_floatarg f1, t_floatarg f2)
{
    t_scaler *x = (t_scaler *)pd_new(scaler_class);
    x->x_mul = f1;
    x->x_add = f2;
	floatinlet_new(&x->x_obj, &x->x_mul);
	floatinlet_new(&x->x_obj, &x->x_add);
	outlet_new(&x->x_obj, &s_signal);
	x->x_f = 0;
    return (x);
}

void scaler_tilde_setup(void)
{
    scaler_class = class_new(gensym("scaler~"), (t_newmethod)scaler_new,
        0, sizeof(t_scaler), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(scaler_class, t_scaler, x_f);
    class_addmethod(scaler_class, (t_method)scaler_dsp,
        gensym("dsp"), A_CANT, 0);
}