#include "shadylib.h"
#include <string.h>

static t_class *siglook_class;

typedef struct _siglook {
	t_object x_obj;
	t_float x_f;
	t_tabtype type;
} t_siglook;

static void *siglook_new(t_symbol *which) {
	t_siglook *x = (t_siglook *)pd_new(siglook_class);
	if(!strcmp(which->s_name, "gauss"))
		x->type = GAUS;
	else if(!strcmp(which->s_name, "cauchy"))
		x->type = CAUCH;
	else x->type = REXP;
	outlet_new(&x->x_obj, &s_signal);
	x->x_f = 0;
	return (x);
}

static void look_rexp(t_siglook *x) {
	x->type = REXP;
}

static void look_cauchy(t_siglook *x){
	x->type = CAUCH;
}

static void look_gauss(t_siglook *x){
	x->type = GAUS;
}

static t_int *siglook_perform(t_int *w) {
	t_tabtype type = *(t_tabtype *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	while(n--) *out++ = readtab(type, *in++);
	return(w + 5);
}

static void siglook_dsp(t_siglook *x, t_signal **sp) {
	dsp_add(siglook_perform, 4, &x->type, sp[0]->s_vec,
		sp[1]->s_vec, sp[0]->s_n);
}

void shadylook_tilde_setup(void) {
	siglook_class = class_new(gensym("shadylook~"), 
		(t_newmethod)siglook_new,
		0, sizeof(t_siglook), 0, A_DEFSYMBOL, 0);
	if(!tabmade) maketabs();
	class_addmethod(siglook_class, (t_method)siglook_dsp, 
		gensym("dsp"), A_CANT, 0);
	CLASS_MAINSIGNALIN(siglook_class, t_siglook, x_f);
	class_addmethod(siglook_class, (t_method)look_cauchy, gensym("cauchy"), 
		0);
	class_addmethod(siglook_class, (t_method)look_gauss, gensym("gauss"), 
		0);
	class_addmethod(siglook_class, (t_method)look_rexp, gensym("rexp"), 
		0);
}