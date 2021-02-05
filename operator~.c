#include "shadylib.h"

static t_class *operator_class;

typedef struct _operator {
	t_object x_obj;
	t_float x_f;
	union shadylib_floatpoint invals[2];
	int num; /* number of msg inlets: 0, 1, or 2 
		0 is all signals, 1 is an add inlet,
		2 is a multiply inlet and an add inlet */
	double x_phase;
    float x_conv;
} t_operator;

static t_int *op_perf0(t_int *w) {
	t_operator *x = (t_operator *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_sample *add = x->invals[1].vec;
	t_sample *addr, f1, f2, frac;
    double dphase = x->x_phase + SHADYLIB_UNITBIT32;
    union shadylib_tabfudge tf;
    float conv = x->x_conv;
    int normhipart;
    tf.tf_d = SHADYLIB_UNITBIT32;
    normhipart = tf.tf_i[SHADYLIB_HIOFFSET];
	    tf.tf_d = dphase;
	    #ifdef FP_FAST_FMA
	    dphase = fma(*in++, conv, dphase);
	    #else
        dphase += *in++ * conv;
        #endif
        addr = shadylib_sintbl + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
        frac = tf.tf_d - SHADYLIB_UNITBIT32;
    while (--n)
    {
        tf.tf_d = dphase;
            f1 = addr[0];
        #ifdef FP_FAST_FMA
	    dphase = fma(*in++, conv, dphase);
	    #else
        dphase += *in++ * conv;
        #endif
            f2 = addr[1];
        addr = shadylib_sintbl + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
        	#ifdef FP_FAST_FMA
        	f1 = fma(frac, f2 - f1, f1);
            *out++ = fma(f1, (*mul++), (*add++));
        	#else
            f1 = f1 + frac * (f2 - f1);
            *out++ = f1*(*mul++) + (*add++);
            #endif
        frac = tf.tf_d - SHADYLIB_UNITBIT32;
    }
            f1 = addr[0];
            f2 = addr[1];
            
            #ifdef FP_FAST_FMA
        	f1 = fma(frac, f2 - f1, f1);
            *out++ = fma(f1, (*mul++), (*add++));
        	#else
            f1 = f1 + frac * (f2 - f1);
            *out++ = f1*(*mul++) + (*add++);
            #endif

    tf.tf_d = SHADYLIB_UNITBIT32 * SHADYLIB_BUZZSIZE;
    normhipart = tf.tf_i[SHADYLIB_HIOFFSET];
    tf.tf_d = dphase + (SHADYLIB_UNITBIT32 * SHADYLIB_BUZZSIZE - SHADYLIB_UNITBIT32);
    tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
    x->x_phase = tf.tf_d - SHADYLIB_UNITBIT32 * SHADYLIB_BUZZSIZE;
    return (w+5);
}

static t_int *op_perf1(t_int *w) {
	t_operator *x = (t_operator *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_float add = x->invals[1].val;
	t_sample *addr, f1, f2, frac;
    double dphase = x->x_phase + SHADYLIB_UNITBIT32;
    union shadylib_tabfudge tf;
    float conv = x->x_conv;
	int normhipart;
    tf.tf_d = SHADYLIB_UNITBIT32;
    normhipart = tf.tf_i[SHADYLIB_HIOFFSET];
	    tf.tf_d = dphase;
        #ifdef FP_FAST_FMA
	    dphase = fma(*in++, conv, dphase);
	    #else
        dphase += *in++ * conv;
        #endif
        addr = shadylib_sintbl + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
        frac = tf.tf_d - SHADYLIB_UNITBIT32;
    while (--n)
    {
        tf.tf_d = dphase;
            f1 = addr[0];
        #ifdef FP_FAST_FMA
	    dphase = fma(*in++, conv, dphase);
	    #else
        dphase += *in++ * conv;
        #endif
            f2 = addr[1];
        addr = shadylib_sintbl + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
            #ifdef FP_FAST_FMA
        	f1 = fma(frac, f2 - f1, f1);
            *out++ = fma(f1, (*mul++), add);
        	#else
            f1 = f1 + frac * (f2 - f1);
            *out++ = f1*(*mul++) + add;
            #endif
        frac = tf.tf_d - SHADYLIB_UNITBIT32;
    }
            f1 = addr[0];
            f2 = addr[1];
            #ifdef FP_FAST_FMA
        	f1 = fma(frac, f2 - f1, f1);
            *out++ = fma(f1, (*mul++), add);
        	#else
            f1 = f1 + frac * (f2 - f1);
            *out++ = f1*(*mul++) + add;
            #endif

    tf.tf_d = SHADYLIB_UNITBIT32 * SHADYLIB_BUZZSIZE;
    normhipart = tf.tf_i[SHADYLIB_HIOFFSET];
	tf.tf_d = dphase + (SHADYLIB_UNITBIT32 * SHADYLIB_BUZZSIZE - SHADYLIB_UNITBIT32);
    tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
    x->x_phase = tf.tf_d - SHADYLIB_UNITBIT32 * SHADYLIB_BUZZSIZE;
    return (w+5);
}

static t_int *op_perf2(t_int *w) {
	t_operator *x = (t_operator *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_float mul = x->invals[0].val;
	t_float add = x->invals[1].val;
	t_sample *addr, f1, f2, frac;
    double dphase = x->x_phase + SHADYLIB_UNITBIT32;
    union shadylib_tabfudge tf;
    float conv = x->x_conv;
	   int normhipart;
    tf.tf_d = SHADYLIB_UNITBIT32;
    normhipart = tf.tf_i[SHADYLIB_HIOFFSET];
	    tf.tf_d = dphase;
        #ifdef FP_FAST_FMA
	    dphase = fma(*in++, conv, dphase);
	    #else
        dphase += *in++ * conv;
        #endif
        addr = shadylib_sintbl + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
        frac = tf.tf_d - SHADYLIB_UNITBIT32;
    while (--n)
    {
        tf.tf_d = dphase;
            f1 = addr[0];
        #ifdef FP_FAST_FMA
	    dphase = fma(*in++, conv, dphase);
	    #else
        dphase += *in++ * conv;
        #endif
            f2 = addr[1];
        addr = shadylib_sintbl + (tf.tf_i[SHADYLIB_HIOFFSET] & (SHADYLIB_BUZZSIZE-1));
        tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
            #ifdef FP_FAST_FMA
        	f1 = fma(frac, f2 - f1, f1);
            *out++ = fma(f1, mul, add);
        	#else
            f1 = f1 + frac * (f2 - f1);
            *out++ = f1*(mul) + add;
            #endif
        frac = tf.tf_d - SHADYLIB_UNITBIT32;
    }
            f1 = addr[0];
            f2 = addr[1];
            #ifdef FP_FAST_FMA
        	f1 = fma(frac, f2 - f1, f1);
            *out++ = fma(f1, mul, add);
        	#else
            f1 = f1 + frac * (f2 - f1);
            *out++ = f1*(mul) + add;
            #endif
	
    tf.tf_d = SHADYLIB_UNITBIT32 * SHADYLIB_BUZZSIZE;
    normhipart = tf.tf_i[SHADYLIB_HIOFFSET];
    tf.tf_d = dphase + (SHADYLIB_UNITBIT32 * SHADYLIB_BUZZSIZE - SHADYLIB_UNITBIT32);
    tf.tf_i[SHADYLIB_HIOFFSET] = normhipart;
    x->x_phase = tf.tf_d - SHADYLIB_UNITBIT32 * SHADYLIB_BUZZSIZE;
    return (w+5);
}

static void operator_dsp(t_operator *x, t_signal **sp) {
	x->x_conv = SHADYLIB_BUZZSIZE/sp[0]->s_sr;
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

static void operator_ft1(t_operator *x, t_float f)
{
    x->x_phase = SHADYLIB_BUZZSIZE * (f + 0.25);
}

static void *operator_new(t_symbol *s, int argc, t_atom *argv) {
	t_operator *x = (t_operator *)pd_new(operator_class);
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

void operator_tilde_setup(void)
{
    operator_class = class_new(gensym("operator~"), (t_newmethod)operator_new, 0, sizeof(t_operator), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(operator_class, t_operator, x_f);
    class_addmethod(operator_class, (t_method)operator_dsp, gensym("dsp"), 
    A_CANT, 0);
    class_addmethod(operator_class, (t_method)operator_ft1,
        gensym("ft1"), A_FLOAT, 0);
    shadylib_checkalign();
    class_setfreefn(operator_class, shadylib_freebuzz);
    shadylib_makebuzz();
}
		
