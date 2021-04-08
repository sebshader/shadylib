#include "shadylib.h"

static t_class *triangulator_class;

typedef struct _triangulator {
	t_object x_obj;
	t_float x_f;
	union shadylib_floatpoint invals[2];
	int num; /* number of msg inlets: 0, 1, or 2 
		0 is all signals, 1 is an add inlet,
		2 is a multiply inlet and an add inlet */
	double x_phase;
    float x_conv;
} t_triangulator;

static t_int *op_perf0(t_int *w) {
	t_triangulator *x = (t_triangulator *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_sample *add = x->invals[1].vec;
	t_sample inter;
	double dphase = x->x_phase + (double)SHADYLIB_UNITBIT32;
    union shadylib_tabfudge tf;
    uint32_t casto;
    
    float conv = x->x_conv;
    tf.tf_d = dphase;

    while (n--)
    {
        #ifdef FP_FAST_FMA
        dphase = fma(*in++, conv, dphase);
        #else
        dphase += *in++ * conv;
        #endif
        casto = (uint32_t)tf.tf_i[SHADYLIB_LOWOFFSET];
        if(casto & 2147483648) /* bit 31 */
        	casto = ~casto;
        inter = (t_sample)casto/1073741823.5 - 1;
        #ifdef FP_FAST_FMA
        *out++ = fma(inter, *mul++, *add++);
        #else
        *out++ = inter*(*mul++) + (*add++);
        #endif
        tf.tf_d = dphase;
    }
    tf.tf_i[SHADYLIB_HIOFFSET] = SHADYLIB_NORMHIPART;
    x->x_phase = tf.tf_d - SHADYLIB_UNITBIT32;
    return (w+5);
}

static t_int *op_perf1(t_int *w) {
	t_triangulator *x = (t_triangulator *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_float add = x->invals[1].val;
	t_sample inter;
	double dphase = x->x_phase + (double)SHADYLIB_UNITBIT32;
    union shadylib_tabfudge tf;
    uint32_t casto;
    
    float conv = x->x_conv;
    tf.tf_d = dphase;

    while (n--)
    {
        #ifdef FP_FAST_FMA
        dphase = fma(*in++, conv, dphase);
        #else
        dphase += *in++ * conv;
        #endif
        casto = (uint32_t)tf.tf_i[SHADYLIB_LOWOFFSET];
        if(casto & 2147483648) /* bit 31 */
        	casto = ~casto;
        inter = (t_sample)casto/1073741823.5 - 1;
        #ifdef FP_FAST_FMA
        *out++ = fma(inter, *mul++, add);
        #else
        *out++ = inter*(*mul++) + add;
        #endif
        tf.tf_d = dphase;
    }
    tf.tf_i[SHADYLIB_HIOFFSET] = SHADYLIB_NORMHIPART;
    x->x_phase = tf.tf_d - SHADYLIB_UNITBIT32;
    return (w+5);
}

static t_int *op_perf2(t_int *w) {
	t_triangulator *x = (t_triangulator *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_float mul = x->invals[0].val;
	t_float add = x->invals[1].val;
	t_sample inter;
	double dphase = x->x_phase + (double)SHADYLIB_UNITBIT32;
    union shadylib_tabfudge tf;
    uint32_t casto;
    
    float conv = x->x_conv;
    tf.tf_d = dphase;

    while (n--)
    {
        #ifdef FP_FAST_FMA
        dphase = fma(*in++, conv, dphase);
        #else
        dphase += *in++ * conv;
        #endif
        casto = (uint32_t)tf.tf_i[SHADYLIB_LOWOFFSET];
        if(casto & 2147483648) /* bit 31 */
        	casto = ~casto;
        inter = (t_sample)casto/1073741823.5 - 1;
        #ifdef FP_FAST_FMA
        *out++ = fma(inter, mul, add);
        #else
        *out++ = inter*mul + add;
        #endif
        tf.tf_d = dphase;
    }
    tf.tf_i[SHADYLIB_HIOFFSET] = SHADYLIB_NORMHIPART;
    x->x_phase = tf.tf_d - SHADYLIB_UNITBIT32;
    return (w+5);
}

static void triangulator_dsp(t_triangulator *x, t_signal **sp) {
	x->x_conv = 1/sp[0]->s_sr;
	switch(x->num) {
		case 0:
			x->invals[0].vec = sp[1]->s_vec;
			x->invals[1].vec = sp[2]->s_vec;
			dsp_add(op_perf0, 4, x, sp[0]->s_vec, sp[3]->s_vec, sp[0]->s_n);
			break;
		case 1:
			x->invals[0].vec = sp[1]->s_vec;
			dsp_add(op_perf1, 4, x, sp[0]->s_vec, sp[2]->s_vec, sp[0]->s_n);
			break;
		case 2:
			dsp_add(op_perf2, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
			break;
		default:;
	}
}

static void triangulator_ft1(t_triangulator *x, t_float f)
{
    x->x_phase = f + .25;
}

static void *triangulator_new(t_symbol* UNUSED(s), int argc, t_atom *argv) {
	t_triangulator *x = (t_triangulator *)pd_new(triangulator_class);
	x->x_phase = 0.25;
	outlet_new(&x->x_obj, &s_signal);
	switch(argc) {
		default:;
		case 3:
			x->x_f = atom_getfloatarg(2, argc, argv);
		case 2:
			floatinlet_new(&x->x_obj, &x->invals[0].val);
			x->invals[0].val = atom_getfloatarg(0, argc, argv);
			floatinlet_new(&x->x_obj, &x->invals[1].val);
			x->invals[1].val = atom_getfloatarg(1, argc, argv);
			x->num = 2;
			break;
		case 1:
			inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
			floatinlet_new(&x->x_obj, &x->invals[1].val);
			x->invals[1].val = atom_getfloatarg(0, argc, argv);
			x->num = 1;
			break;
		case 0:
			inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
			inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
			x->num = 0;
	}
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
	return(x);
}

	

void triangulator_tilde_setup(void)
{
    triangulator_class = class_new(gensym("triangulator~"), (t_newmethod)triangulator_new, 0, sizeof(t_triangulator), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(triangulator_class, t_triangulator, x_f);
    class_addmethod(triangulator_class, (t_method)triangulator_dsp, gensym("dsp"), 
    A_CANT, 0);
    class_addmethod(triangulator_class, (t_method)triangulator_ft1,
        gensym("ft1"), A_FLOAT, 0);
    shadylib_checkalign();
}
		
