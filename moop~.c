#include "shadylib.h"
/* taken mostly from pd source code */

typedef struct _moopbuf {
	int x_npoints;//stuff for reading the array
    t_float x_onset;
    t_word *x_vec;
} t_moopbuf;


typedef struct _moop {
	t_object x_obj;
	t_float x_f;
	t_moopbuf x_buf; //buffer info
	t_symbol *x_arrayname;
    t_float x_tperiod; //period without samplerate
    t_float x_tempo; //number to * the period in ms by
    t_float x_trns; //held transposition
    t_float x_sr; //samplerate
    t_sample x_range; //sample to jump to on negative times
    int x_num; //current number of repetitions
    int x_hold; //sample & hold transposition or not?
    int x_inf; //loop indefinitely or not?
    double x_phase; //keep track of time passed
    double x_period; //inverse of samples to play
    double x_sample; //actual sample value to read in buffer
} t_moop;

static t_class *moop_class;

static void moop_tempo(t_moop *x, t_floatarg tempo) {
	x->x_tempo = (tempo > 0) ? tempo : 1;
}

static void moop_time(t_moop *x, t_symbol *s, int argc, t_atom *argv) {
	t_float time;
	if(argc > 0) {
		if(argv[0].a_type == A_FLOAT) {
			time = atom_getfloat(argv);
			if(time == 0.0) {
				x->x_num = 0;
				return;
			} else {
				x->x_tperiod = 1000/(time*(x->x_tempo));
				x->x_period = x->x_tperiod/x->x_sr;
			}
		} else {
			post("moop~: time must be float");
			return;
		}
		if(argc > 1) 
			if(argv[1].a_type == A_FLOAT) {
				t_int num = atom_getfloat(argv + 1);
				x->x_inf = 0;
				x->x_phase = 0.0;
				if(time < 0.0) x->x_sample = x->x_range;
				else x->x_sample = 0.0;
				if(num <= 0) {
					x->x_num = 0;
					x->x_phase = 0.0;
					x->x_sample = 0.0;
				} else {
					x->x_num = num;
				}
			} else {
				x->x_inf = 1;
				x->x_num = 1;
			}
		else {
			x->x_sample = (time < 0.0) ? x->x_range : 0.;
			x->x_phase = 0.0;
			x->x_inf = 0;
			x->x_num = 1;
		}
	}
}

static void moop_range(t_moop *x, t_floatarg range) {
	if(range < 0) range = 0;
	x->x_range = range + 1;
}

static void moop_period(t_moop *x, t_floatarg period) {
	x->x_tperiod = 1000/(period*(x->x_tempo));
	x->x_period = x->x_tperiod/x->x_sr;
}

void moop_tilde_set(t_moop *x, t_symbol *s) {
    t_garray *a;
    int points;
    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
        if (*s->s_name)
            pd_error(x, "moop~: %s: no such array", x->x_arrayname->s_name);
        x->x_buf.x_vec = 0;
    }
    else if (!garray_getfloatwords(a, &points, &x->x_buf.x_vec))
    {
        pd_error(x, "%s: bad template for moop~", x->x_arrayname->s_name);
        x->x_buf.x_vec = 0;
    } else {
    	x->x_buf.x_npoints = points - 4;
    	garray_usedindsp(a);
    }
}

static void moop_hold(t_moop *x, t_floatarg hold) {
	x->x_hold = (hold != 0.0);
}

static t_sample moop_rd(t_moopbuf buf, double findex) {
	double a1, a2, a3;
	long iindex;
	t_word *wp;
	t_sample a, b, c, d;
	findex += buf.x_onset;
	iindex = findex;
	if (iindex < 0)
		iindex = 0, findex = 0;
	else if (iindex > buf.x_npoints)
		iindex = buf.x_npoints, findex = 1;
	else findex -= iindex;
	wp = iindex + buf.x_vec;
	a = wp[0].w_float;
    b = wp[1].w_float;
    c = wp[2].w_float;
    d = wp[3].w_float;
	// 4-point, 3rd-order Hermite (x-form)
	a1 = 0.5f * (c - a);
	a2 = a - 2.5 * b + 2.f * c - 0.5f * d;
	a3 = 0.5f * (d - a) + 1.5f * (b - c);

	return ((a3 * findex + a2) * findex + a1) * findex + b;
}

static t_int *moop_perform(t_int *w) {
	t_moop *x = (t_moop *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *in2 = (t_sample *)(w[3]);
	t_sample *out1 = (t_sample *)(w[4]);
	t_sample *out2 = (t_sample *)(w[5]);
	t_sample *out3 = (t_sample *)(w[6]);
	int n = (int)(w[7]);
	t_sample range = x->x_range;
	double period = x->x_period, sample = x->x_sample;
	int num = x->x_num, caster;
	double phase = x->x_phase;
	t_moopbuf buffer = x->x_buf;
    t_sample tin, tin2, tout;
    /* just to store sign (avoid fabs) */
    float sgn = copysign(1, period);
    period = fabs(period);
    
    if(buffer.x_vec) {
    	tout = moop_rd(buffer, sample);
    } else {
    	tout = 0;
    	goto done;
    }
	if(!num) goto done;
	if(x->x_hold) {
		t_float trns=x->x_trns;
		for(;n;--n) {
			
			tin = *in++;
			tin2 = *in2++;
			if (phase == 0.) {
				buffer.x_onset = tin2;
				trns = copysign(exp2(tin/12), sgn);
			}
			tout = moop_rd(buffer, sample);
			*out1++ = tout;
    		*out2++ = phase;
    		*out3++ = sample;
    		phase += period;
    		sample += trns;
    		if (phase >= 1.) {
    			if(!x->x_inf) {
    				num--;
					if(!num) {
						phase = 0.0;
						n--;
						goto done;
					}
				}
				caster = phase;
				phase = phase - caster;
				trns = copysign(exp2(tin/12), sgn);
				sample = (sgn > 0) ? 0 : range;
				buffer.x_onset = tin2;
			}	
		}
		x->x_trns = trns;
	} else {
		for(;n;--n) {
			
			tin = *in++;
			tin2 = *in2++;
			if (phase == 0.) {
				buffer.x_onset = tin2;
			}
			tout = moop_rd(buffer, sample);
			*out1++ = tout;
    		*out2++ = phase;
    		*out3++ = sample;
    		phase += period;
    		sample += copysign(exp2(tin/12), sgn);
    		if (phase >= 1.) {
    			if(!x->x_inf) {
    				num--;
					if(!num) {
						phase = 0.0;
						n--;
						goto done;
					}
				}
				caster = phase;
				phase = phase - caster;
				sample = (sgn > 0) ? 0 : range;
				buffer.x_onset = tin2;
			}	
		}
	}
	done:
	
	while (n--) {
		*out1++ = tout;
		*out2++ = phase;
		*out3++ = sample;
	}
	x->x_num = num;
	x->x_buf = buffer;
	x->x_sample = sample;
	x->x_phase = phase;
    return (w+8);
}

static void moop_dsp(t_moop *x, t_signal **sp) {
	moop_tilde_set(x, x->x_arrayname);
	x->x_sr = sp[0]->s_sr;
	x->x_period = x->x_tperiod/x->x_sr;
	dsp_add(moop_perform, 7, x, sp[0]->s_vec, sp[1]->s_vec, 
		sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
}

static void *moop_new(t_floatarg range, t_floatarg hold, t_symbol *s) {
	t_moop *x = (t_moop *)pd_new(moop_class);
	outlet_new(&x->x_obj, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);

    moop_range(x, range);
	moop_hold(x, hold);
	x->x_tempo = 1;
	x->x_sr = 1.;
	x->x_arrayname = s;
	x->x_phase = 0.0;
	x->x_tperiod = 0.0;
	x->x_sample = 0.0;
	x->x_num = 0;
	x->x_f = 0;
    x->x_buf.x_onset = 0;
    x->x_buf.x_vec = 0;
	return (x);
}

void moop_tilde_setup(void) {
	moop_class = class_new(gensym("moop~"), (t_newmethod)moop_new, 0,
		sizeof(t_moop), 0, A_DEFFLOAT, A_DEFFLOAT, A_DEFSYM, 0);
	class_addmethod(moop_class, (t_method)moop_dsp, gensym("dsp"), A_CANT, 0);
	CLASS_MAINSIGNALIN(moop_class, t_moop, x_f);
	class_addmethod(moop_class, (t_method)moop_tilde_set,
        gensym("set"), A_SYMBOL, 0);
	class_addmethod(moop_class, (t_method)moop_hold, gensym("hold"),
		A_DEFFLOAT, 0);
	class_addmethod(moop_class, (t_method)moop_period, gensym("period"),
		A_DEFFLOAT, 0);
	class_addmethod(moop_class, (t_method)moop_time, gensym("times"),
		A_GIMME, 0);
	class_addmethod(moop_class, (t_method)moop_range, gensym("range"),
		A_DEFFLOAT, 0);
	class_addmethod(moop_class, (t_method)moop_tempo, gensym("tempo"),
		A_DEFFLOAT, 0);
}
