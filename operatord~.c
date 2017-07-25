#include "shadylib.h"

static t_class *operatord_class;

typedef struct _operatord {
	t_object x_obj;
	t_float x_f;
	union floatpoint invals[2];
	int num; /* number of msg inlets: 0, 1, or 2 
		0 is all signals, 1 is an add inlet,
		2 is a multiply inlet and an add inlet */
} t_operatord;

static t_int *op_perf0(t_int *w) {
	t_operatord *x = (t_operatord *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_sample *add = x->invals[1].vec;
	t_sample *tab = sintbl, *addr, f1, f2, frac;
	
    double dphase;
    int normhipart;
    union tabfudge tf;
    
    tf.tf_d = UNITBIT32;
    normhipart = tf.tf_i[HIOFFSET];


        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
        tf.tf_d = dphase;
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
        tf.tf_i[HIOFFSET] = normhipart;
    while (--n)
    {
        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
            frac = tf.tf_d - UNITBIT32;
        tf.tf_d = dphase;
            f1 = addr[0];
            f2 = addr[1];
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
        	#ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, (*mul++), (*add++));
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + (*add++);
            #endif
        tf.tf_i[HIOFFSET] = normhipart;
    }
            frac = tf.tf_d - UNITBIT32;
            f1 = addr[0];
            f2 = addr[1];
            #ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, (*mul++), (*add++));
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + (*add++);
            #endif
    return (w+5);
}

static t_int *op_perf1(t_int *w) {
	t_operatord *x = (t_operatord *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_float add = x->invals[1].val;
	t_sample *tab = sintbl, *addr, f1, f2, frac;
	
    double dphase;
    int normhipart;
    union tabfudge tf;
    
    tf.tf_d = UNITBIT32;
    normhipart = tf.tf_i[HIOFFSET];


        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
        tf.tf_d = dphase;
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
        tf.tf_i[HIOFFSET] = normhipart;
    while (--n)
    {
        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
            frac = tf.tf_d - UNITBIT32;
        tf.tf_d = dphase;
            f1 = addr[0];
            f2 = addr[1];
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
        #ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, (*mul++), add);
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + add;
            #endif
        tf.tf_i[HIOFFSET] = normhipart;
    }
            frac = tf.tf_d - UNITBIT32;
            f1 = addr[0];
            f2 = addr[1];
            #ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, (*mul++), add);
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*(*mul++) + add;
            #endif
    return (w+5);
}

static t_int *op_perf2(t_int *w) {
	t_operatord *x = (t_operatord *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_float mul = x->invals[0].val;
	t_float add = x->invals[1].val;
	t_sample *tab = sintbl, *addr, f1, f2, frac;
	
    double dphase;
    int normhipart;
    union tabfudge tf;
    
    tf.tf_d = UNITBIT32;
    normhipart = tf.tf_i[HIOFFSET];


        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
        tf.tf_d = dphase;
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
        tf.tf_i[HIOFFSET] = normhipart;
    while (--n)
    {
        dphase = (double)(*in++ * (float)(BUZZSIZE)) + UNITBIT32;
            frac = tf.tf_d - UNITBIT32;
        tf.tf_d = dphase;
            f1 = addr[0];
            f2 = addr[1];
        addr = tab + (tf.tf_i[HIOFFSET] & (BUZZSIZE-1));
    		#ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, mul, add);
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*mul + add;
            #endif
        tf.tf_i[HIOFFSET] = normhipart;
    }
            frac = tf.tf_d - UNITBIT32;
            f1 = addr[0];
            f2 = addr[1];
            #ifdef FP_FAST_FMA
        	dphase = fma(frac, f2 - f1, f1);
            *out++ = fma(dphase, mul, add);
        	#else
            dphase = f1 + frac * (f2 - f1);
            *out++ = dphase*mul + add;
            #endif
    return (w+5);
}

static void operatord_dsp(t_operatord *x, t_signal **sp) {
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

static void *operatord_new(t_symbol *s, int argc, t_atom *argv) {
	t_operatord *x = (t_operatord *)pd_new(operatord_class);
	outlet_new(&x->x_obj, &s_signal);
	switch(argc) {
		default:;
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
	return(x);
}

void operatord_tilde_setup(void)
{
    operatord_class = class_new(gensym("operatord~"), (t_newmethod)operatord_new, 0, sizeof(t_operatord), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(operatord_class, t_operatord, x_f);
    class_addmethod(operatord_class, (t_method)operatord_dsp, gensym("dsp"), 
    A_CANT, 0);
    checkalign();
    makebuzz();
}
		
