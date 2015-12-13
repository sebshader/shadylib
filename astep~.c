#include "m_pd.h"

static t_class *astep_tilde_class;

typedef struct _astep {
	t_object x_obj;
	double x_msecpersamp;
	t_float x_msec;
	double x_current;
} t_astep;

static t_int *astep_tilde_perform(t_int *w) {
	t_astep *x = (t_astep *)(w[1]);
	t_sample *out = (t_sample *)(w[2]);
	int n = (int)(w[3]);
	double mspers = x->x_msecpersamp;
	t_float limit = x->x_msec;
	double current = x->x_current;
	if(current >= limit)
		while(n--) *out++ = 0.;
	else {
		while(n--) {
			if(current >= limit)
				*out++ = 0.;
			else {
				*out++ = 1.;
				current += mspers;
			}
		}
		x->x_current = current;
	}
	return (w + 4);
}

static void astep_tilde_float(t_astep *x, t_floatarg f) {
	x->x_current = 0.;
	x->x_msec = f;
}

static void astep_tilde_dsp(t_astep *x, t_signal **sp)
{
    dsp_add(astep_tilde_perform, 3, x, sp[0]->s_vec, sp[0]->s_n);
    x->x_msecpersamp = ((double)1000) / sp[0]->s_sr;
}

static void *astep_tilde_new(void) {
    t_astep *x = (t_astep *)pd_new(astep_tilde_class);
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

void astep_tilde_setup(void) {
	astep_tilde_class = class_new(gensym("astep~"), (t_newmethod)astep_tilde_new, 0,
        sizeof(t_astep), 0, 0);
    class_addmethod(astep_tilde_class, (t_method)astep_tilde_dsp, gensym("dsp"), 
    	A_CANT, 0);
    class_addfloat(astep_tilde_class, astep_tilde_float);
}