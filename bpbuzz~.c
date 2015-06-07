#include "shadylib.h"
#include <float.h>
#include <string.h>

static t_class *bpbuzz_class, *scalarbpbuzz_class;
/* this algorithm is mainly from supercollider
should be modified for 16-bit ints;
*/

typedef struct _bpbuzz {
	t_object x_obj;
    t_float x_f;
    double phase;
    double x_conv;
    float max;
} t_bpbuzz;

typedef struct _scalarbpbuzz {
	t_object x_obj;
    t_float x_f;
    t_float x_g;
    t_float x_duty;
    int argset; /* flag to tell if x_g was set to be 1/2 sr
    			this gets unset the first time x_g is changed */
    double phase;
    double x_conv;
    float max;
} t_scalarbpbuzz;

static void scalarbpbuzz_phase(t_scalarbpbuzz *x, t_float f) {
	f *= .5;
	x->phase = f - floorf(f);
}

static void bpbuzz_phase(t_bpbuzz *x, t_float f) {
	f *= .5;
	x->phase = f - floorf(f);
}

static void scalarbpbuzz_freq(t_scalarbpbuzz *x, t_float f) {
	x->x_g = fminf(x->max, fmaxf(f, 0));
	x->argset = 0;
}

static void *bpbuzz_new(t_symbol *s, int argc, t_atom *argv)
{
    if (argc > 2) post("bpbuzz~: extra arguments ignored");
    t_float duty = (argc > 1) ? atom_getfloatarg(1, argc, argv) : .5;
    if (argc) 
    {
    	if(argv->a_type == A_SYMBOL &&
    		!strcmp(argv->a_w.w_symbol->s_name, "-d"))
    			goto signal;	
        t_scalarbpbuzz *x = (t_scalarbpbuzz *)pd_new(scalarbpbuzz_class);
        signalinlet_new(&x->x_obj, duty);
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("freq"));
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("phase"));
        x->max = FLT_MAX;
        if (argv->a_type == A_FLOAT) {
        	x->x_g = atom_getfloatarg(0, argc, argv);
        	x->argset = 0;
        } else {
        	x->x_g = x->max;
        	x->argset = 1;
        }
        outlet_new(&x->x_obj, &s_signal);
        outlet_new(&x->x_obj, &s_signal);
        outlet_new(&x->x_obj, &s_signal);
        x->x_f = 0;
        return (x);
    }
    signal:
    {
        t_bpbuzz *x = (t_bpbuzz *)pd_new(bpbuzz_class);
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        signalinlet_new(&x->x_obj, duty);
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("phase"));
        outlet_new(&x->x_obj, &s_signal);
        outlet_new(&x->x_obj, &s_signal);
        outlet_new(&x->x_obj, &s_signal);
        x->x_f = 0;
        return (x);
    }
}

static t_int *scalarbpbuzz_perform(t_int *w) {
	t_scalarbpbuzz *x = (t_scalarbpbuzz *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *in2 = (t_sample *)(w[3]);
	t_sample *out1 = (t_sample *)(w[4]);
    t_sample *out2 = (t_sample *)(w[5]);
    t_sample *out3 = (t_sample *)(w[6]);
    int n = (int)(w[7]);
    double conv = x->x_conv;
    double phase = x->phase;
    double rat, frat, res1, res2, res3, res4, fread;
    t_sample freq, duty, final, final2;;
    uint32_t tabrd, tabrd2, n2;
    float g = x->x_g;
    float max = x->max;
	while(n--) {
		freq = fmin(fabs(*in++), max);
		duty = fmin(fmax(*in2++, .001), .999);
		duty = .5*(1 - duty);
		fread = phase*BUZZSIZE;
		tabrd = fread;
		tabrd2 = tabrd + 1;
		res1 = cosectbl[tabrd];
		res2 = cosectbl[tabrd2];
		res3 = fread - tabrd;
		if(res1 == BADVAL || res2 == BADVAL) {
			res1 = sintbl[tabrd];
			res2 = sintbl[tabrd2];
			res2 = res1 + (res2 - res1)*res3;
			if(fabs(res2)  < 0.0005f) {
				final = 1;
				rat = g/freq;
				if (rat > MAXHARM) {
					rat = MAXHARM;
				} else {
					frat = max/freq - 1;
					n2 = frat;
					if(rat > n2) rat = frat;
				}
				rat = fmax(rat, 1);
				n2 = rat;
				frat = rat - n2;
				n2 *= 2;
				goto gotfinal;
			} else res2 = 1/res2;
		} else res2 = res1 + (res2 - res1)*res3;
		rat = g/freq;
		if (rat > MAXHARM) {
			rat = MAXHARM;
		} else {
			frat = max/freq - 1;
			n2 = frat;
			if(rat > n2) rat = frat;
		}
		rat = fmax(rat, 1);
		n2 = rat;
		frat = rat - n2;
		n2 *= 2;
		res4 = fread*(n2 + 1);
		tabrd = res4;
		res1 = res4 - tabrd;
		tabrd %= BUZZSIZE;
		res3 = sintbl[tabrd];
		tabrd++;
		res3 = res3 + (sintbl[tabrd] - res3)*res1;
		res3 = (res3*res2 - 1)/n2;
		res1 = res4 + fread*2;
		tabrd = res1;
		res4 = res1 - tabrd;
		tabrd = tabrd % BUZZSIZE;
		res1 = sintbl[tabrd];
		tabrd++;
		res1 = res1 + (sintbl[tabrd] - res1)*res4;
		res1 = (res1*res2 - 1)/(n2 + 2);
		final = res3*(1 - frat) + res1*(frat);

gotfinal:
		
		fread = phase + duty;
		tabrd = fread;
		fread = fread - tabrd;
		fread *= BUZZSIZE;
		tabrd = fread;
		tabrd2 = tabrd + 1;
		res1 = cosectbl[tabrd];
		res2 = cosectbl[tabrd2];
		res3 = fread - tabrd;
		if(res1 == BADVAL || res2 == BADVAL) {
			res1 = sintbl[tabrd];
			res2 = sintbl[tabrd2];
			res2 = res1 + (res2 - res1)*res3;
			if(fabs(res2)  < 0.0005f) {
				final2 = -1;
				goto gotfinal2;
			} else res2 = 1/res2;
		} else res2 = res1 + (res2 - res1)*res3;
		res4 = fread*(n2 + 1);
		tabrd = res4;
		res1 = res4 - tabrd;
		tabrd %= BUZZSIZE;
		res3 = sintbl[tabrd];
		tabrd++;
		res3 = res3 + (sintbl[tabrd] - res3)*res1;
		res3 = (res3*res2 - 1)/n2;
		res1 = res4 + fread*2;
		tabrd = res1;
		res4 = res1 - tabrd;
		tabrd = tabrd % BUZZSIZE;
		res1 = sintbl[tabrd];
		tabrd++;
		res1 = res1 + (sintbl[tabrd] - res1)*res4;
		res1 = (res1*res2 - 1)/(n2 + 2);
		final2 = res3*(frat - 1) - res1*(frat);
gotfinal2:
		
		*out1++ = (final + final2);
		*out2++ = final;
		//(1/rat + 1);
		res1 = phase*2;
		n2 = res1;
		*out3++ = res1 - n2;
		phase += freq*conv;
		if(phase >= 1) {
			n2 = phase;
			phase = phase - n2;
		}
	}
	x->phase = phase;
	return(w + 8);
}
		
static t_int *bpbuzz_perform(t_int *w) {
	t_bpbuzz *x = (t_bpbuzz *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *in2 = (t_sample *)(w[3]);
	t_sample *in3 = (t_sample *)(w[4]);
	t_sample *out1 = (t_sample *)(w[5]);
    t_sample *out2 = (t_sample *)(w[6]);
    t_sample *out3 = (t_sample *)(w[7]);
    int n = (int)(w[8]);
    double conv = x->x_conv;
    double phase = x->phase;
    double rat, frat, res1, res2, res3, res4, fread;
    t_sample freq, duty, final, final2;
    float max = x->max;
    uint32_t tabrd, tabrd2, n2;
    t_sample g;
	while(n--) {
		freq = fmin(fabs(*in++), max);
		g = fmin(fmax(*in2++, 0.), max);
		duty = fmin(fmax(*in3++, .001), .999);
		duty = .5*(1 - duty);
		fread = phase*BUZZSIZE;
		tabrd = fread;
		tabrd2 = tabrd + 1;
		res1 = cosectbl[tabrd];
		res2 = cosectbl[tabrd2];
		res3 = fread - tabrd;
		if(res1 == BADVAL || res2 == BADVAL) {
			res1 = sintbl[tabrd];
			res2 = sintbl[tabrd2];
			res2 = res1 + (res2 - res1)*res3;
			if(fabs(res2)  < 0.0005f) {
				final = 1;
				rat = g/freq;
				if (rat > MAXHARM) {
					rat = MAXHARM;
				} else {
					frat = max/freq - 1;
					n2 = frat;
					if(rat > n2) rat = frat;
				}
				rat = fmax(rat, 1);
				n2 = rat;
				frat = rat - n2;
				n2 *= 2;
				goto gotfinal;
			} else res2 = 1/res2;
		} else res2 = res1 + (res2 - res1)*res3;
		rat = g/freq;
		if (rat > MAXHARM) {
			rat = MAXHARM;
		} else {
			frat = max/freq - 1;
			n2 = frat;
			if(rat > n2) rat = frat;
		}
		rat = fmax(rat, 1);
		n2 = rat;
		frat = rat - n2;
		n2 *= 2;
		res4 = fread*(n2 + 1);
		tabrd = res4;
		res1 = res4 - tabrd;
		tabrd %= BUZZSIZE;
		res3 = sintbl[tabrd];
		tabrd++;
		res3 = res3 + (sintbl[tabrd] - res3)*res1;
		res3 = (res3*res2 - 1)/n2;
		res1 = res4 + fread*2;
		tabrd = res1;
		res4 = res1 - tabrd;
		tabrd = tabrd % BUZZSIZE;
		res1 = sintbl[tabrd];
		tabrd++;
		res1 = res1 + (sintbl[tabrd] - res1)*res4;
		res1 = (res1*res2 - 1)/(n2 + 2);
		final = res3*(1 - frat) + res1*(frat);

gotfinal:
		fread = phase + duty;
		tabrd = fread;
		fread = fread - tabrd;
		fread *= BUZZSIZE;
		tabrd = fread;
		tabrd2 = tabrd + 1;
		res1 = cosectbl[tabrd];
		res2 = cosectbl[tabrd2];
		res3 = fread - tabrd;
		if(res1 == BADVAL || res2 == BADVAL) {
			res1 = sintbl[tabrd];
			res2 = sintbl[tabrd2];
			res2 = res1 + (res2 - res1)*res3;
			if(fabs(res2)  < 0.0005f) {
				final2 = -1;
				goto gotfinal2;
			} else res2 = 1/res2;
		} else res2 = res1 + (res2 - res1)*res3;
		n2 = rat;
		frat = rat - n2;
		n2 *= 2;
		res4 = fread*(n2 + 1);
		tabrd = res4;
		res1 = res4 - tabrd;
		tabrd %= BUZZSIZE;
		res3 = sintbl[tabrd];
		tabrd++;
		res3 = res3 + (sintbl[tabrd] - res3)*res1;
		res3 = (res3*res2 - 1)/n2;
		res1 = res4 + fread*2;
		tabrd = res1;
		res4 = res1 - tabrd;
		tabrd = tabrd % BUZZSIZE;
		res1 = sintbl[tabrd];
		tabrd++;
		res1 = res1 + (sintbl[tabrd] - res1)*res4;
		res1 = (res1*res2 - 1)/(n2 + 2);

		final2 = res3*(frat - 1) - res1*(frat);
gotfinal2:
		*out1++ = (final + final2);
		*out2++ = final;
		res1 = phase*2;
		n2 = res1;
		*out3++ = res1 - n2;
		phase += freq*conv;
		if(phase >= 1) {
			n2 = phase;
			phase = phase - n2;
		}
	}
	x->phase = phase;
	return(w + 9);
}

static void bpbuzz_dsp(t_bpbuzz *x, t_signal **sp)
{
	x->max = sp[0]->s_sr/2;
    x->x_conv = 0.5/sp[0]->s_sr;
    dsp_add(bpbuzz_perform, 8, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
    	sp[3]->s_vec, sp[4]->s_vec, sp[5]->s_vec, sp[0]->s_n);
}

static void scalarbpbuzz_dsp(t_scalarbpbuzz *x, t_signal **sp)
{
	float max = sp[0]->s_sr/2;
	if (x->max != max) {
		x->max = max;
		if (x->argset) x->x_g = max;
		else if(x->x_g > max) x->x_g = max;
	}
	x->x_conv = 0.5/sp[0]->s_sr;
    dsp_add(scalarbpbuzz_perform, 7, x, sp[0]->s_vec, sp[1]->s_vec, 
    	sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
}

void bpbuzz_tilde_setup(void)
{
    bpbuzz_class = class_new(gensym("bpbuzz~"), (t_newmethod)bpbuzz_new, 0,
        sizeof(t_bpbuzz), 0, A_GIMME, 0);
    class_addmethod(bpbuzz_class, (t_method)bpbuzz_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(bpbuzz_class, t_bpbuzz, x_f);
    class_addmethod(bpbuzz_class, (t_method)bpbuzz_phase,
        gensym("phase"), A_FLOAT, 0);
    scalarbpbuzz_class = class_new(gensym("bpbuzz~"), 0, 0,
        sizeof(t_scalarbpbuzz), 0, 0);
    CLASS_MAINSIGNALIN(scalarbpbuzz_class, t_scalarbpbuzz, x_f);
    class_addmethod(scalarbpbuzz_class, (t_method)scalarbpbuzz_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(scalarbpbuzz_class, (t_method)scalarbpbuzz_freq,
        gensym("freq"), A_FLOAT, 0);
    class_addmethod(scalarbpbuzz_class, (t_method)scalarbpbuzz_phase,
        gensym("phase"), A_FLOAT, 0);
	if(!buzzmade) maketables();
}