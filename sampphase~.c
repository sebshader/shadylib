#include "shadylib.h"
/* modified from pd source */
/* in the style of R. Hoeldrich (ICMC 1995 Banff) */

/* -------------------------- sampphase~ ------------------------------ */
static t_class *sampphase_class;

typedef struct _sampphase
{
    t_object x_obj;
    int x_samptrue;
    double x_phase;
    float x_conv;
    t_sample x_held;
    t_float x_f;      /* scalar frequency */
} t_sampphase;

static void *sampphase_new(t_floatarg f, t_floatarg t)
{
    t_sampphase *x = (t_sampphase *)pd_new(sampphase_class);
    x->x_samptrue = (t == 0);
    x->x_f = f;
    x->x_held = f;
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    x->x_phase = 0;
    outlet_new(&x->x_obj, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static t_int *sampphase_perform(t_int *w)
{
    t_sampphase *x = (t_sampphase *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out1 = (t_sample *)(w[3]);
    t_sample *out2 = (t_sample *)(w[4]);
    int n = (int)(w[5]);
    int bool = x->x_samptrue;
    union shadylib_tabfudge tf;
    tf.tf_d = x->x_phase + (double)SHADYLIB_UNITBIT32;
	float conv = x->x_conv;
	
    if (bool) {
    	t_sample samp = x->x_held;
    	while (n--) {
    		if ((*in != 0.0 && samp == 0.0) ||\
				 (tf.tf_i[SHADYLIB_HIOFFSET] != SHADYLIB_NORMHIPART)) samp = *in;
			in++;
			tf.tf_i[SHADYLIB_HIOFFSET] = SHADYLIB_NORMHIPART;
    		*out1++ = tf.tf_d - SHADYLIB_UNITBIT32;
    		*out2++ = samp;
    		#ifdef FP_FAST_FMA
    		tf.tf_d = fma(samp, conv, tf.tf_d);
    		#else
    		tf.tf_d += samp * conv;
    		#endif
		}
		x->x_held = samp;
    } else {
    	double dphase = tf.tf_d;
    	while (n--) {
    		tf.tf_i[SHADYLIB_HIOFFSET] = SHADYLIB_NORMHIPART;
    		#ifdef FP_FAST_FMA
    		dphase = fma(*in, conv, dphase);
    		#else
        	dphase += *in * conv;
        	#endif
        	*out2++ = *in++;
        	*out1++ = tf.tf_d - SHADYLIB_UNITBIT32;
        	tf.tf_d = dphase;
    	}
	}
    tf.tf_i[SHADYLIB_HIOFFSET] = SHADYLIB_NORMHIPART;
   	x->x_phase = tf.tf_d - SHADYLIB_UNITBIT32;
    return (w+6);
}

static void sampphase_dsp(t_sampphase *x, t_signal **sp)
{ 
    x->x_conv = 1./sp[0]->s_sr;
    dsp_add(sampphase_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

static void sampphase_ft1(t_sampphase *x, t_float f)
{
    x->x_phase = f;
}

static void sampphase_hold(t_sampphase *x, t_float f)
{
    x->x_samptrue = (f != 0.);
}

void sampphase_tilde_setup(void)
{
    sampphase_class = class_new(gensym("sampphase~"), (t_newmethod)sampphase_new, 0, sizeof(t_sampphase), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(sampphase_class, t_sampphase, x_f);
    class_addmethod(sampphase_class, (t_method)sampphase_dsp, gensym("dsp"), 
    A_CANT, 0);
    class_addmethod(sampphase_class, (t_method)sampphase_ft1,
        gensym("ft1"), A_FLOAT, 0);
    class_addmethod(sampphase_class, (t_method)sampphase_hold,
        gensym("hold"), A_FLOAT, 0);
    shadylib_checkalign();
}