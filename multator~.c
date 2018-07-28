#include "shadylib.h"

static t_class *multator_class;

typedef struct _multator {
	t_object x_obj;
	t_float x_f;
	t_sample last;
	union floatpoint invals[2];
	int num; /* number of msg inlets: 0, 1, or 2 
		0 is all signals, 1 is an add inlet,
		2 is a multiply inlet and an add inlet */
	double x_phase;
    float x_conv;
    int rwave;
} t_multator;

static t_int *op_perf0(t_int *w) {
	t_multator *x = (t_multator *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *in2 = (t_sample *)(w[3]);
	t_sample *out = (t_sample *)(w[4]);
	t_sample *out2 = (t_sample *)(w[5]);
	int n = (int)(w[6]);
	t_sample *mul = x->invals[0].vec;
	t_sample *add = x->invals[1].vec;
	t_sample *addr, f1, f2, frac;
	t_sample last = x->last;
    double dphase = x->x_phase;
    float conv = x->x_conv;
    int wavein;
    int rwave = x->rwave;
	unsigned int caster;
	
	decider:
	switch (rwave) {
		default:;
		case 0:
			goto sine;
		case 1:
			goto square;
		case 2:
			goto triangle;
		case 3:
			goto sawtooth;
	}
	
    sine:
    while (n) {
        #ifdef FP_FAST_FMA
        frac = dphase*BUZZSIZE;
        caster = frac;
        frac = frac - caster;
        addr = sintbl + (caster & (BUZZSIZE-1));
        f1 = addr[0];
        f2 = addr[1];
        last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    f1 = fma(frac, f2 - f1, f1);
	    wavein = *in2++;
		*out++ = fma(f1, (*mul++), (*add++));
	    #else
        frac = dphase*BUZZSIZE;
        caster = frac;
        frac = frac - caster;
        addr = sintbl + (caster & (BUZZSIZE-1));
        f1 = addr[0];
        f2 = addr[1];
        last = dphase;
	    dphase += fabs(*in++)*conv;
	    f1 = f1 + frac * (f2 - f1);
	    wavein = *in2++;
		*out++ = f1*(*mul++) + (*add++);
        #endif
        caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
    }
        
    square:
	while (n) {
		#ifdef FP_FAST_FMA
		f1 = fma((dphase < .5), 2,  -1);
		last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    wavein = *in2++;
		*out++ = fma(f1, (*mul++), (*add++));
		#else
		f1 = (dphase < .5)*2  - 1;
		last = dphase;
	    dphase += fabs(*in++*conv);
	    wavein = *in2++;
		*out++ = *mul++*f1 + *add++;
		#endif
		caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
		
    }
    
	triangle:
	while (n) {
		f1 = dphase + .75;
		caster = f1;
		f1 = f1 - caster;
		#ifdef FP_FAST_FMA
		f1 = fma(fabs(f1 - 0.5), 4, -1);
		last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    wavein = *in2++;
		*out++ = fma(f1, (*mul++), (*add++));
		#else
		f1 = fabs(f1 - 0.5)*4 - 1;
		last = dphase;
	    dphase += fabs(*in++*conv);
	    wavein = *in2++;
		*out++ = *mul++*f1 + *add++;
		#endif
		caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
		
    }
	
	sawtooth:
	while (n) {
		f1 = dphase + .5;
		caster = f1;
		f1 = f1 - caster;
		#ifdef FP_FAST_FMA
		f1 = fma(f1, 2, -1);
		last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    wavein = *in2++;
		*out++ = fma(f1, (*mul++), (*add++));
		#else
		f1 = f1*2 - 1;
		last = dphase;
	    dphase += fabs(*in++*conv);
	    wavein = *in2++;
		*out++ = *mul++*f1 + *add++;
		#endif
		caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
		
    }
	
    x->x_phase = dphase;
    x->last = last;
    x->rwave = rwave;
    return (w+7);
}

static t_int *op_perf1(t_int *w) {
	t_multator *x = (t_multator *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *in2 = (t_sample *)(w[3]);
	t_sample *out = (t_sample *)(w[4]);
	t_sample *out2 = (t_sample *)(w[5]);
	int n = (int)(w[6]);
	t_sample *mul = x->invals[0].vec;
	t_float add = x->invals[1].val;
	t_sample *addr, f1, f2, frac;
	t_sample last = x->last;
    double dphase = x->x_phase;
    float conv = x->x_conv;
    int wavein;
    int rwave = x->rwave;
	unsigned int caster;
	
	decider:
	switch (rwave) {
		default:;
		case 0:
			goto sine;
		case 1:
			goto square;
		case 2:
			goto triangle;
		case 3:
			goto sawtooth;
	}
	
    sine:
    while (n) {
        #ifdef FP_FAST_FMA
        frac = dphase*BUZZSIZE;
        caster = frac;
        frac = frac - caster;
        addr = sintbl + (caster & (BUZZSIZE-1));
        f1 = addr[0];
        f2 = addr[1];
        last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    f1 = fma(frac, f2 - f1, f1);
	    wavein = *in2++;
		*out++ = fma(f1, (*mul++), add);
	    #else
        frac = dphase*BUZZSIZE;
        caster = frac;
        frac = frac - caster;
        addr = sintbl + (caster & (BUZZSIZE-1));
        f1 = addr[0];
        f2 = addr[1];
	    dphase += fabs(*in++)*conv;
	    f1 = f1 + frac * (f2 - f1);
	    wavein = *in2++;
		*out++ = f1*(*mul++) + add;
        #endif
        caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
    }
	
    square:
	while (n) {
		#ifdef FP_FAST_FMA
		f1 = fma((dphase < .5), 2,  -1);
		last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    wavein = *in2++;
		*out++ = fma(f1, (*mul++), add);
		#else
		f1 = (dphase < .5)*2  - 1;
		last = dphase;
	    dphase += fabs(*in++*conv);
	    wavein = *in2++;
		*out++ = *mul++*f1 + add;
		#endif
		caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
		
    }
    
	triangle:
	while (n) {
		f1 = dphase + .75;
		caster = f1;
		f1 = f1 - caster;
		#ifdef FP_FAST_FMA
		f1 = fma(fabs(f1 - 0.5), 4, -1);
		last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    wavein = *in2++;
		*out++ = fma(f1, (*mul++), add);
		#else
		f1 = fabs(f1 - 0.5)*4 - 1;
		last = dphase;
	    dphase += fabs(*in++*conv);
	    wavein = *in2++;
		*out++ = *mul++*f1 + add;
		#endif
		caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
		
    }
	
	sawtooth:
	while (n) {
		f1 = dphase + .5;
		caster = f1;
		f1 = f1 - caster;
		#ifdef FP_FAST_FMA
		f1 = fma(f1, 2, -1);
		last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    wavein = *in2++;
		*out++ = fma(f1, (*mul++), add);
		#else
		f1 = f1*2 - 1;
		last = dphase;
	    dphase += fabs(*in++*conv);
	    wavein = *in2++;
		*out++ = *mul++*f1 + add;
		#endif
		caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
		
    }
    x->x_phase = dphase;
    x->last = last;
    x->rwave = rwave;
    return (w+7);
}

static t_int *op_perf2(t_int *w) {
	t_multator *x = (t_multator *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *in2 = (t_sample *)(w[3]);
	t_sample *out = (t_sample *)(w[4]);
	t_sample *out2 = (t_sample *)(w[5]);
	int n = (int)(w[6]);
	t_float mul = x->invals[0].val;
	t_float add = x->invals[1].val;
	t_sample *addr, f1, f2, frac;
	t_sample last = x->last;
    double dphase = x->x_phase;
    float conv = x->x_conv;
    int wavein;
    int rwave = x->rwave;
	unsigned int caster;
	
	decider:
	switch (rwave) {
		default:;
		case 0:
			goto sine;
		case 1:
			goto square;
		case 2:
			goto triangle;
		case 3:
			goto sawtooth;
	}
	
    sine:
    while (n) {
        #ifdef FP_FAST_FMA
        frac = dphase*BUZZSIZE;
        caster = frac;
        frac = frac - caster;
        addr = sintbl + (caster & (BUZZSIZE-1));
        f1 = addr[0];
        f2 = addr[1];
        last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    f1 = fma(frac, f2 - f1, f1);
	    wavein = *in2++;
		*out++ = fma(f1, mul, add);
	    #else
        frac = dphase*BUZZSIZE;
        caster = frac;
        frac = frac - caster;
        addr = sintbl + (caster & (BUZZSIZE-1));
        f1 = addr[0];
        f2 = addr[1];
        last = dphase;
	    dphase += fabs(*in++)*conv;
	    f1 = f1 + frac * (f2 - f1);
	    wavein = *in2++;
		*out++ = f1*mul + add;
        #endif
        caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
    }
        
    square:
	while (n) {
		#ifdef FP_FAST_FMA
		f1 = fma((dphase < .5), 2,  -1);
		last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    wavein = *in2++;
		*out++ = fma(f1, mul, add);
		#else
		f1 = (dphase < .5)*2  - 1;
		last = dphase;
	    dphase += fabs(*in++*conv);
	    wavein = *in2++;
		*out++ = mul*f1 + add;
		#endif
		caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
		
    }
    
	triangle:
	while (n) {
		f1 = dphase + .75;
		caster = f1;
		f1 = f1 - caster;
		#ifdef FP_FAST_FMA
		f1 = fma(fabs(f1 - 0.5), 4, -1);
		last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    wavein = *in2++;
		*out++ = fma(f1, mul, add);
		#else
		f1 = fabs(f1 - 0.5)*4 - 1;
		last = dphase;
	    dphase += fabs(*in++*conv);
	    wavein = *in2++;
		*out++ = mul*f1 + add;
		#endif
		caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
		
    }
	
	sawtooth:
	while (n) {
		f1 = dphase + .5;
		caster = f1;
		f1 = f1 - caster;
		#ifdef FP_FAST_FMA
		f1 = fma(f1, 2, -1);
		last = dphase;
	    dphase = fma(fabs(*in++), conv, dphase);
	    wavein = *in2++;
		*out++ = fma(f1, mul, add);
		#else
		f1 = f1*2 - 1;
		last = dphase;
	    dphase += fabs(*in++*conv);
	    wavein = *in2++;
		*out++ = mul*f1 + add;
		#endif
		caster = dphase;
        dphase = dphase - caster;
		*out2++ = last;
		
        n--;
        if (rwave != wavein && dphase < last) {
        	rwave = wavein;
        	goto decider;
        }
		
    }
	
    x->x_phase = dphase;
    x->last = last;
    x->rwave = rwave;
    return (w+7);
}

static void multator_dsp(t_multator *x, t_signal **sp) {
	x->x_conv = 1/sp[0]->s_sr;
	switch(x->num) {
		case 0:
			x->invals[0].vec = sp[1]->s_vec;
			x->invals[1].vec = sp[2]->s_vec;
			dsp_add(op_perf0, 6, x, sp[0]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec, sp[0]->s_n);
			break;
		case 1:
			x->invals[0].vec = sp[1]->s_vec;
			dsp_add(op_perf1, 6, x, sp[0]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
			break;
		case 2:
			dsp_add(op_perf2, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[3]->s_vec, sp[0]->s_n);
			break;
		default:;
	}
}

static void multator_ft1(t_multator *x, t_float f)
{
	int caster;
	f += 0.;
	caster = f;
    x->x_phase = f - caster;
}

static void *multator_new(t_symbol *s, int argc, t_atom *argv) {
	t_multator *x = (t_multator *)pd_new(multator_class);
	x->x_phase = 0.;
	outlet_new(&x->x_obj, &s_signal);
	x->rwave = (argc >= 4) ? atom_getfloatarg(3, argc, argv) : 0;
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
	pd_float(
        (t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal),
            x->rwave);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
	outlet_new(&x->x_obj, gensym("signal"));
	return(x);
}

void multator_tilde_setup(void)
{
    multator_class = class_new(gensym("multator~"), (t_newmethod)multator_new, 0, sizeof(t_multator), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(multator_class, t_multator, x_f);
    class_addmethod(multator_class, (t_method)multator_dsp, gensym("dsp"), 
    A_CANT, 0);
    class_addmethod(multator_class, (t_method)multator_ft1,
        gensym("ft1"), A_FLOAT, 0);
    checkalign();
    makebuzz();
}
		
