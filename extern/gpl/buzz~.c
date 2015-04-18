#include "m_pd.h"
#include <math.h>

static t_class *buzz_class, *scalarbuzz_class;
static t_sample *sintbl;
static t_sample *cosectbl;
/* this algorithm is mainly from supercollider
should be modified for 16-bit ints;

used in the cosecant table for values very close to 1/0 */
#define BADVAL 1e20f

/* size of tables */
#define BUZZSIZE 8192

/* maximum harmonics for small frequencies */
#define MAXHARM ((int)((4294967295/(2*BUZZSIZE)) - 2))

/* create sine and cosecant tables */
static void maketables(void) {
	sintbl = (t_sample *)getbytes(sizeof(t_sample) * (BUZZSIZE + 1)*2);
	cosectbl = sintbl + BUZZSIZE + 1;
	double incr = 2*M_PI/BUZZSIZE;
	double phase;
	double res;
	int i;
	for(i = 0; i <= BUZZSIZE; i++) {
		phase = i*incr;
		sintbl[i] = sin(phase);
		cosectbl[i] = 1/sintbl[i];
	}
	/* insert BADVALs */
	cosectbl[0] = cosectbl[BUZZSIZE/2] = cosectbl[BUZZSIZE] = BADVAL;
	for (i=1; i<=8; ++i) {
		cosectbl[i] = cosectbl[BUZZSIZE-i] = BADVAL;
		cosectbl[BUZZSIZE/2-i] = cosectbl[BUZZSIZE/2+i] = BADVAL;
	}
}

typedef struct _buzz {
	t_object x_obj;
    t_float x_f;
    double phase;
    double x_conv;
    float max;
} t_buzz;

typedef struct _scalarbuzz {
	t_object x_obj;
    t_float x_f;
    t_float x_g;
    int argset; /* flag to tell if x_g was set to be 1/2 sr
    			this gets unset the first time x_g is changed */
    double phase;
    double x_conv;
    float max;
} t_scalarbuzz;

static void scalarbuzz_phase(t_scalarbuzz *x, t_float f) {
	x->phase = f - floorf(f);
}

static void buzz_phase(t_buzz *x, t_float f) {
	x->phase = f - floorf(f);
}

static void scalarbuzz_freq(t_scalarbuzz *x, t_float f) {
	x->x_g = fminf(x->max, fmaxf(f, 0));
	x->argset = 0;
}

static void *buzz_new(t_symbol *s, int argc, t_atom *argv)
{
    if (argc > 1) post("buzz~: extra arguments ignored");
    if (argc) 
    {
        t_scalarbuzz *x = (t_scalarbuzz *)pd_new(scalarbuzz_class);
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("freq"));
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("phase"));
        x->max = sys_getsr()/2.;
        if (argv->a_type == A_FLOAT) {
        	x->x_g = atom_getfloatarg(0, argc, argv);
        	x->argset = 0;
        } else {
        	x->x_g = x->max;
        	x->argset = 1;
        }
        outlet_new(&x->x_obj, &s_signal);
        outlet_new(&x->x_obj, &s_signal);
        x->x_f = 0;
        return (x);
    }
    else
    {
        t_buzz *x = (t_buzz *)pd_new(buzz_class);
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("phase"));
        outlet_new(&x->x_obj, &s_signal);
        outlet_new(&x->x_obj, &s_signal);
        x->x_f = 0;
        return (x);
    }
}

static t_int *scalarbuzz_perform(t_int *w) {
	t_scalarbuzz *x = (t_scalarbuzz *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out1 = (t_sample *)(w[3]);
    t_sample *out2 = (t_sample *)(w[4]);
    int n = (int)(w[5]);
    double conv = x->x_conv;
    double phase = x->phase;
    double rat, frat, res1, res2, res3, res4, fread;
    t_sample freq;
    uint32_t tabrd, tabrd2, n2;
    float g = x->x_g;
    float max = x->max;
	while(n--) {
		freq = fmin(fabs(*in++), max);
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
				*out1++ = 1;
				res1 = phase*2;
				n2 = res1;
				*out2++ = res1 - n2;
				phase += freq*conv;
				if(phase >= 1) {
					n2 = phase;
					phase = phase - n2;
				}
				continue;
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
		*out1++ = res3*(1 - frat) + res1*(frat);
		res1 = phase*2;
		n2 = res1;
		*out2++ = res1 - n2;
		phase += freq*conv;
		if(phase >= 1) {
			n2 = phase;
			phase = phase - n2;
		}
	}
	x->phase = phase;
	return(w + 6);
}
		
static t_int *buzz_perform(t_int *w) {
	t_buzz *x = (t_buzz *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *in2 = (t_sample *)(w[3]);
	t_sample *out1 = (t_sample *)(w[4]);
    t_sample *out2 = (t_sample *)(w[5]);
    int n = (int)(w[6]);
    double conv = x->x_conv;
    double phase = x->phase;
    double rat, frat, res1, res2, res3, res4, fread;
    float max = x->max;
    t_sample freq;
    uint32_t tabrd, tabrd2, n2;
    t_sample g;
	while(n--) {
		freq = fmin(fabs(*in++), max);
		g = fmin(fmax(*in2++, 0.), max);
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
				*out1++ = 1;
				res1 = phase*2;
				n2 = res1;
				*out2++ = res1 - n2;
				phase += freq*conv;
				if(phase >= 1) {
					n2 = phase;
					phase = phase - n2;
				}
				continue;
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
		*out1++ = res3*(1 - frat) + res1*(frat);
		res1 = phase*2;
		n2 = res1;
		*out2++ = res1 - n2;
		phase += freq*conv;
		if(phase >= 1) {
			n2 = phase;
			phase = phase - n2;
		}
	}
	x->phase = phase;
	return(w + 7);
}

static void buzz_dsp(t_buzz *x, t_signal **sp)
{
	x->max = sp[0]->s_sr/2;
    x->x_conv = 0.5/sp[0]->s_sr;
    dsp_add(buzz_perform, 6, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
    	sp[3]->s_vec, sp[0]->s_n);
}

static void scalarbuzz_dsp(t_scalarbuzz *x, t_signal **sp)
{
	float max = sp[0]->s_sr/2;
	if (x->max != max) {
		x->max = max;
		if (x->argset) x->x_g = max;
		else if(x->x_g > max) x->x_g = max;
	}
	x->x_conv = 0.5/sp[0]->s_sr;
    dsp_add(scalarbuzz_perform, 5, x, sp[0]->s_vec,
            sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

void buzz_tilde_setup(void)
{
    buzz_class = class_new(gensym("buzz~"), (t_newmethod)buzz_new, 0,
        sizeof(t_buzz), 0, A_GIMME, 0);
    class_addmethod(buzz_class, (t_method)buzz_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(buzz_class, t_buzz, x_f);
    class_addmethod(buzz_class, (t_method)buzz_phase,
        gensym("phase"), A_FLOAT, 0);
    scalarbuzz_class = class_new(gensym("buzz~"), 0, 0,
        sizeof(t_scalarbuzz), 0, 0);
    CLASS_MAINSIGNALIN(scalarbuzz_class, t_scalarbuzz, x_f);
    class_addmethod(scalarbuzz_class, (t_method)scalarbuzz_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(scalarbuzz_class, (t_method)scalarbuzz_freq,
        gensym("freq"), A_FLOAT, 0);
    class_addmethod(scalarbuzz_class, (t_method)scalarbuzz_phase,
        gensym("phase"), A_FLOAT, 0);
    
	maketables();
}