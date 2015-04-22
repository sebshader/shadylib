#include "m_pd.h"

static t_class *inrange_class;

typedef struct _inrange {
	t_object x_obj;
	/* 0 means 1st inlet lo, 1 means 1st inlet hi */
	int x_state;
	t_float x_lo;
	t_float x_hi;
	t_outlet *f_out1, *f_out2;
} t_inrange;

static void inrange_float(t_inrange *x, t_floatarg in) {
	if(in >= x->x_lo && in <= x->x_hi)
		outlet_float(x->f_out1, in);
	else outlet_float(x->f_out2, in);
}
	
static void inrange_f1(t_inrange *x, t_floatarg in) {
	if(x->x_state)
		if(in >= x->x_lo) x->x_hi = in;
		else {
			x->x_hi = x->x_lo;
			x->x_lo = in;
			x->x_state = 0;
		}
	else {
		if(in <= x->x_hi) x->x_lo = in;
		else {
			x->x_lo = x->x_hi;
			x->x_hi = in;
			x->x_state = 1;
		}
	}		
}

static void inrange_f2(t_inrange *x, t_floatarg in) {
	if(x->x_state)
		if(in <= x->x_hi) x->x_lo = in;
		else {
			x->x_lo = x->x_hi;
			x->x_hi = in;
			x->x_state = 0;
		}
	else {
		if(in >= x->x_lo) x->x_hi = in;
		else {
			x->x_hi = x->x_lo;
			x->x_lo = in;
			x->x_state = 1;
		}
	}		
}
	
static void *inrange_new(t_floatarg f1, t_floatarg f2)
{
    t_inrange *x = (t_inrange *)pd_new(inrange_class);
    x->x_state = 0;
    x->x_lo = 0.0;
    x->x_hi = 0.0;
    inrange_f2(x, f2);
    inrange_f1(x, f1);
    x->f_out1 = outlet_new(&x->x_obj,&s_float);
    x->f_out2 = outlet_new(&x->x_obj,&s_float);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("f1"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("f2"));
    return (x);
}

void inrange_setup(void)
{
    inrange_class = class_new(gensym("inrange"), (t_newmethod)inrange_new,
        0, sizeof(t_inrange), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(inrange_class, (t_method)inrange_f1,
        gensym("f1"), A_FLOAT, 0);
    class_addmethod(inrange_class, (t_method)inrange_f2,
        gensym("f2"), A_FLOAT, 0);
    class_addfloat(inrange_class, (t_method)inrange_float);
}