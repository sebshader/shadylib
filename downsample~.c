#include "shadylib.h"

static t_class* downsample_tilde_class;

typedef struct _downsample_tilde {
	t_object x_obj;

	t_float x_f;
	t_sample x_held;
	double x_phase;
    float x_conv;
} t_downsample_tilde;

static void downsample_tilde_set(t_downsample_tilde* x, t_float phase) {
    x->x_phase = shadylib_max(0.0, phase);
}

static t_int *downsample_tilde_perform(t_int *w) {
    t_downsample_tilde* x = (t_downsample_tilde*)(w[1]);
	t_sample *in1 = (t_sample*) (w[2]), *in2 = (t_sample*) (w[3]),
		*out = (t_sample*) (w[4]);
	int n = (int) (w[5]);
	double dphase = x->x_phase;
    float conv = x->x_conv;
    t_sample rate, held = x->x_held, temp_in1;

    while (n--)
    {
        temp_in1 = *in1++;
        rate = *in2++;
        
        if(dphase >= 1.0) {
            held = temp_in1;
            dphase = dphase - (int)dphase;
        }
        dphase += shadylib_max(rate, 0.0) * conv;
        *out++ = held;
    }
    x->x_held = held;
    x->x_phase = dphase;
	return (w + 6);
}

static void downsample_tilde_dsp(t_downsample_tilde* x, t_signal **sp) {
    x->x_conv = 1/sp[0]->s_sr;
	dsp_add(downsample_tilde_perform, 5, x, sp[0]->s_vec,  sp[1]->s_vec,
		sp[2]->s_vec, sp[0]->s_n);
}

static void* downsample_tilde_new(t_symbol* UNUSED(s), int argc, t_atom *argv) {
	t_downsample_tilde* x = (t_downsample_tilde*) pd_new(downsample_tilde_class);
	/* unfortunately no way to tell if we've received a new value from the inlet
	 it seems (or else we could track the samplerate when it changes)*/
	signalinlet_new(&x->x_obj, argc ? atom_getfloatarg(0, argc, argv) :
		sys_getsr());
	
	x->x_phase = 1.0;
	x->x_held = 0.0;
	
	outlet_new(&x->x_obj, &s_signal);
	return (void*) x;
}

void downsample_tilde_setup(void) {
	downsample_tilde_class = class_new(gensym("downsample~"), 
		(t_newmethod)downsample_tilde_new, 0, sizeof(t_downsample_tilde),
		CLASS_DEFAULT, A_GIMME, 0);
	class_addmethod(downsample_tilde_class, (t_method)downsample_tilde_dsp,
	    gensym("dsp"), A_CANT, 0);
	class_addmethod(downsample_tilde_class, (t_method)downsample_tilde_set,
	    gensym("set"), A_FLOAT, 0);
	CLASS_MAINSIGNALIN(downsample_tilde_class, t_downsample_tilde, x_f);
}