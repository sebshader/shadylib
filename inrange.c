#include "m_pd.h"

static t_class *inrange_class;

typedef struct _inrange {
	t_object x_obj;
	t_float x_left;
	t_float x_right;
	t_outlet *f_out1, *f_out2;
} t_inrange;

static void inrange_float(t_inrange *x, t_floatarg in) {
    /* in >= x->x_lo && in <= x->x_hi */
	if((x->x_left - in) * (x->x_right - in) <= 0.0)
		outlet_float(x->f_out1, in);
	else outlet_float(x->f_out2, in);
}

static void *inrange_new(t_floatarg f1, t_floatarg f2)
{
    t_inrange *x = (t_inrange *)pd_new(inrange_class);
    x->x_left = f1;
    x->x_right = f2;
    x->f_out1 = outlet_new(&x->x_obj,&s_float);
    x->f_out2 = outlet_new(&x->x_obj,&s_float);
    floatinlet_new(&x->x_obj, &x->x_left);
    floatinlet_new(&x->x_obj, &x->x_right);
    return (x);
}

void inrange_setup(void)
{
    inrange_class = class_new(gensym("inrange"), (t_newmethod)inrange_new,
        0, sizeof(t_inrange), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(inrange_class, (t_method)inrange_float);
}

