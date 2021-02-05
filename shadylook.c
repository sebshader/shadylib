#include "shadylib.h"
#include <string.h>

static t_class *shadylook_class;


typedef struct _look {
	t_object x_obj;
	t_shadylib_tabtype type;
} t_look;

static void *shadylook_new(t_symbol *which) {
	t_look *x = (t_look *)pd_new(shadylook_class);
	if(!strcmp(which->s_name, "gauss"))
		x->type = GAUS;
	else if(!strcmp(which->s_name, "cauchy"))
		x->type = CAUCH;
	else x->type = REXP;
	outlet_new(&x->x_obj, &s_float);
	return (x);
}

static void look_rexp(t_look *x) {
	x->type = REXP;
}

static void look_cauchy(t_look *x){
	x->type = CAUCH;
}

static void look_gauss(t_look *x){
	x->type = GAUS;
}

static void look_float(t_look *x, t_floatarg dex) {
	outlet_float(x->x_obj.ob_outlet, shadylib_readtab(x->type, dex));
}

void shadylook_setup(void) {
	shadylook_class = class_new(gensym("shadylook"), (t_newmethod)shadylook_new,
		0, sizeof(t_look), 0, A_DEFSYMBOL, 0);
	class_addmethod(shadylook_class, (t_method)look_cauchy, gensym("cauchy"), 
		0);
	class_addmethod(shadylook_class, (t_method)look_gauss, gensym("gauss"), 
		0);
	class_addmethod(shadylook_class, (t_method)look_rexp, gensym("rexp"), 
		0);
	class_addfloat(shadylook_class, (t_method)look_float);
	class_setfreefn(shadylook_class, shadylib_freetabs);
	shadylib_maketabs();
}