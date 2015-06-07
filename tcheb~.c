#include "m_pd.h"
#include <math.h>

#define MAX_HARM 254.0
//#define COSTAB_2 (COSTABSIZE >> 1)
static t_class* tcheb_tilde_class;

typedef struct _tcheb_tilde {
	t_object x_obj;
	t_float dumf;
} t_tcheb_tilde;

t_int *tcheb_tilde_perform(t_int *w) {
	t_tcheb_tilde *x = (t_tcheb_tilde*) (w[1]);
	t_sample *in1 = (t_sample*) (w[2]), *in2 = (t_sample*) (w[3]),
		*out = (t_sample*) (w[4]);
	int n = (int) (w[5]);
	int newo, l, dir;
	float inord; //change floorf below for pd-double
	double t1, t2, tin, temp;
	while(n--) {
		inord = *in2++;
		if (inord < 0.0) inord = 0.0;
		else if (inord > MAX_HARM) inord = MAX_HARM;
		newo = inord + 2;
		/*this is basically a binary stack for even-odd computations*/
		for(l = 0, dir = 0; newo > 2; l++, newo = (newo + 1) >> 1) {
				dir <<= 1;
				dir = (1 & newo) | dir;
		}
		t1 = *in1++;
		t1 = fmax(-1.0, fmin(t1, 1.0));
		t2 = 2.0*t1*t1 - 1.0; 
		tin = t1;
		for(int i = 0; i < l; i++) {
			if(dir & 1) {
			/* 2t(n)*t(m) = t(n+m) + t(n-m) for m = n - 1 and n. odd and even
				idea is from fxt */
				temp = 2.0*t1;
				#ifdef FP_FAST_FMA
				t1 = fma(temp, t1, -1.0);
				t2 = fma(temp, t2, -tin);
				#else
				t1 = temp*t1 - 1.0;
				t2 = temp*t2 - tin;
				#endif
			} else {
				temp = 2.0*t2;
				#ifdef FP_FAST_FMA
				t1 = fma(temp, t1, -tin);
				t2 = fma(temp, t2, -1.0);
				#else
				t1 = temp*t1 - tin;
				t2 = temp*t2 - 1.0;
				#endif
			}
			dir >>= 1;
		}
		inord = (inord - floorf(inord));
		#ifdef FP_FAST_FMA
		*out++ = (t_sample)(fma(t1, (1.0 - inord), t2*inord);
		#else
		*out++ = (t_sample)(t1*(1.0 - inord) + t2*inord);
		#endif
	}
	return (w + 6);
}

void tcheb_tilde_dsp(t_tcheb_tilde *x, t_signal **sp) {
	dsp_add(tcheb_tilde_perform, 5, x, sp[0]->s_vec,  sp[1]->s_vec,
		sp[2]->s_vec, sp[0]->s_n);
}

void* tcheb_tilde_new(t_floatarg def) {
	t_tcheb_tilde* x = (t_tcheb_tilde*) pd_new(tcheb_tilde_class);
	
	signalinlet_new(&x->x_obj, def);
	
	outlet_new(&x->x_obj, &s_signal);
	
	return (void*) x;
}
void tcheb_tilde_setup(void) {
	tcheb_tilde_class = class_new(gensym("tcheb~"), 
		(t_newmethod)tcheb_tilde_new, 0, sizeof(t_tcheb_tilde), CLASS_DEFAULT, A_DEFFLOAT, 
		0);
	class_addmethod(tcheb_tilde_class, (t_method)tcheb_tilde_dsp, gensym("dsp"), A_CANT, 0);
	CLASS_MAINSIGNALIN(tcheb_tilde_class, t_tcheb_tilde, dumf);
}