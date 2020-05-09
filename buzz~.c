#include "shadylib.h"
#include <float.h>

static t_class *buzz_class, *scalarbuzz_class;
/* this algorithm is mainly from supercollider
should be modified for 16-bit ints;
*/

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
	f *= .5;
	x->phase = f - floorf(f);
}

static void buzz_phase(t_buzz *x, t_float f) {
	f *= .5;
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
		res1 = shadylib_cosectbl[tabrd];
		res2 = shadylib_cosectbl[tabrd2];
		res3 = fread - tabrd;
		if(res1 == BADVAL || res2 == BADVAL) {
			res1 = shadylib_sintbl[tabrd];
			res2 = shadylib_sintbl[tabrd2];
			#ifdef FP_FAST_FMA
			res2 = fma(res2 - res1, res3, res1);
			#else
			res2 = res1 + (res2 - res1)*res3;
			#endif
			if(fabs(res2)  < 0.0005f) {
				*out1++ = 1;
				*out2++ = phase;
				#ifdef FP_FAST_FMA
				phase = fma(freq, conv, phase);
				#else
				phase += freq*conv;
				#endif
				n2 = phase;
				phase = phase - n2;
				continue;
			} else res2 = 1/res2;
		} else {
			#ifdef FP_FAST_FMA
			res2 = fma(res2 - res1, res3, res1);
			#else
			res2 = res1 + (res2 - res1)*res3;
			#endif
		}
		rat = g/freq - 1;
		rat = fmax(fmin(rat, MAXHARM), 1);
		n2 = rat;
		frat = rat - n2;
		n2 *= 2;
		#ifdef FP_FAST_FMA
		res4 = fma(n2, fread, fread);
		#else
		res4 = fread*(n2 + 1);
		#endif
		tabrd = res4;
		res1 = res4 - tabrd;
		tabrd = tabrd & (BUZZSIZE - 1);
		res3 = shadylib_sintbl[tabrd];
		tabrd++;
		
		#ifdef FP_FAST_FMA
		res3 = fma(shadylib_sintbl[tabrd] - res3, res1, res3);
		res3 = fma(res3, res2, -1)/n2;
		res1 = fma(fread, 2, res4);
		#else
		res3 = res3 + (shadylib_sintbl[tabrd] - res3)*res1;
		res3 = (res3*res2 - 1)/n2;
		res1 = res4 + fread*2;
		#endif

		tabrd = res1;
		res4 = res1 - tabrd;
		tabrd = tabrd & (BUZZSIZE - 1);
		res1 = shadylib_sintbl[tabrd];
		tabrd++;
		
		#ifdef FP_FAST_FMA
		res1 = fma(shadylib_sintbl[tabrd] - res1, res4, res1);
		res1 = fma(res1, res2, -1)/(n2 + 2);
		*out1++ = fma(res3, 1-frat, res1*frat);
		#else
		res1 = res1 + (shadylib_sintbl[tabrd] - res1)*res4;
		res1 = (res1*res2 - 1)/(n2 + 2);
		*out1++ = res3*(1 - frat) + res1*(frat);
		#endif
		
		*out2++ = phase;
		#ifdef FP_FAST_FMA
		phase = fma(freq, conv, phase);
		#else
		phase += freq*conv;
		#endif
		n2 = phase;
		phase = phase - n2;
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
		g = fmin(*in2++, max);
		fread = phase*BUZZSIZE;
		tabrd = fread;
		tabrd2 = tabrd + 1;
		res1 = shadylib_cosectbl[tabrd];
		res2 = shadylib_cosectbl[tabrd2];
		res3 = fread - tabrd;
		if(res1 == BADVAL || res2 == BADVAL) {
			res1 = shadylib_sintbl[tabrd];
			res2 = shadylib_sintbl[tabrd2];
			#ifdef FP_FAST_FMA
			res2 = fma(res2 - res1, res3, res1);
			#else
			res2 = res1 + (res2 - res1)*res3;
			#endif
			if(fabs(res2)  < 0.0005f) {
				*out1++ = 1;
				*out2++ = phase;
				#ifdef FP_FAST_FMA
				phase = fma(freq, conv, phase);
				#else
				phase += freq*conv;
				#endif
				n2 = phase;
				phase = phase - n2;
				continue;
			} else res2 = 1/res2;
		} else {
			#ifdef FP_FAST_FMA
			res2 = fma(res2 - res1, res3, res1);
			#else
			res2 = res1 + (res2 - res1)*res3;
			#endif
		}
		rat = g/freq - 1;
		rat = fmax(fmin(rat, MAXHARM), 1);
		n2 = rat;
		frat = rat - n2;
		n2 *= 2;
		#ifdef FP_FAST_FMA
		res4 = fma(n2, fread, fread);
		#else
		res4 = fread*(n2 + 1);
		#endif
		tabrd = res4;
		res1 = res4 - tabrd;
		tabrd = tabrd & (BUZZSIZE - 1);
		res3 = shadylib_sintbl[tabrd];
		tabrd++;
		
		#ifdef FP_FAST_FMA
		res3 = fma(shadylib_sintbl[tabrd] - res3, res1, res3);
		res3 = fma(res3, res2, -1)/n2;
		res1 = fma(fread, 2, res4);
		#else
		res3 = res3 + (shadylib_sintbl[tabrd] - res3)*res1;
		res3 = (res3*res2 - 1)/n2;
		res1 = res4 + fread*2;
		#endif

		tabrd = res1;
		res4 = res1 - tabrd;
		tabrd = tabrd & (BUZZSIZE - 1);
		res1 = shadylib_sintbl[tabrd];
		tabrd++;
		
		#ifdef FP_FAST_FMA
		res1 = fma(shadylib_sintbl[tabrd] - res1, res4, res1);
		res1 = fma(res1, res2, -1)/(n2 + 2);
		*out1++ = fma(res3, 1-frat, res1*frat);
		#else
		res1 = res1 + (shadylib_sintbl[tabrd] - res1)*res4;
		res1 = (res1*res2 - 1)/(n2 + 2);
		*out1++ = res3*(1 - frat) + res1*(frat);
		#endif

		*out2++ = phase;
		#ifdef FP_FAST_FMA
		phase = fma(freq, conv, phase);
		#else
		phase += freq*conv;
		#endif
		n2 = phase;
		phase = phase - n2;
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
    class_setfreefn(buzz_class, shadylib_freebuzz);
	shadylib_makebuzz();
}