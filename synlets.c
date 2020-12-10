#include "m_pd.h"

static t_class *synlets_class;
static t_class *proxy_class;

typedef struct proxy
{
	t_object obj;
	struct _synlets *x;		/* we'll put the other struct in here */
} t_proxy;

typedef struct _synlets {
	t_object x_obj;
	int x_leftcount;
	int x_rightcount;
	int x_leftdone;
	int x_rightdone;
	int x_leftlim;
	int x_rightlim;
	t_proxy *x_proxy;
	t_outlet *b_out1, *f_out2, *f_out3;
} t_synlets;

static void output(t_synlets *x) {
	x->x_leftcount = 0;
	x->x_rightcount = 0;
	x->x_leftdone = 0;
	x->x_rightdone = 0;
	outlet_float(x->f_out3, 0.0);
	outlet_float(x->f_out2, 0.0);
	outlet_bang(x->b_out1);
}

static void synlets_float(t_synlets *x, t_floatarg in) {
	int llim = in;
	x->x_leftlim = llim;
	if(llim <= x->x_leftcount) {
		if (x->x_rightdone) output(x);
		else x->x_leftdone = 1;
	} else if (x->x_leftdone) x->x_leftdone = 0;
}

static void synlets_bang(t_synlets *x) {
	int count = x->x_leftcount;
	count++;
	if(count >= x->x_leftlim)
		if(x->x_rightdone) {
			output(x);
			return;
		} else x->x_leftdone = 1;
	x->x_leftcount = count;
	outlet_float(x->f_out2, count);
}

static void synlets_set(t_synlets *x, t_floatarg val) {
	int intval = val;
	if (intval >= x->x_leftlim) x->x_leftdone = 1;
	x->x_leftcount = intval;
}

static void proxy_bang(t_proxy *x) {
	int count = x->x->x_rightcount;
	count++;
	if(count >= x->x->x_rightlim)
		if(x->x->x_leftdone) {
			output(x->x);
			return;
		} else x->x->x_rightdone = 1;
	x->x->x_rightcount = count;
	outlet_float(x->x->f_out3, count);
}

static void proxy_float(t_proxy *x, t_floatarg f) {
	int llim = f;
	x->x->x_rightlim = llim;
	if(llim <= x->x->x_rightcount) {
		if (x->x->x_leftdone) output(x->x);
		else x->x->x_rightdone = 1;
	} else if (x->x->x_rightdone) x->x->x_rightdone = 0;
}

static void proxy_set(t_proxy *x, t_floatarg val) {
	int intval = val;
	if (intval >= x->x->x_rightlim) x->x->x_rightdone = 1;
	x->x->x_rightcount = intval;
}

static void *synlets_new(t_symbol *name, int argc, t_atom *argv)
{
    t_synlets *x = (t_synlets *)pd_new(synlets_class);
	x->x_leftcount = 0;
	x->x_rightcount = 0;
	x->x_leftdone = 0;
	x->x_rightdone = 0;
	x->x_proxy = (t_proxy *)pd_new(proxy_class);
	x->x_proxy->x = x;
	switch(argc) {
		case 0:
			x->x_leftlim = 1;
		case 1:
			x->x_rightlim = 1;
		default:;
	}
	switch(argc) {
		default:
			x->x_rightlim = atom_getint(argv+1);
		case 1:
			x->x_leftlim = atom_getint(argv);
		case 0:;
	}
    x->b_out1 = outlet_new(&x->x_obj,&s_bang);
    x->f_out2 = outlet_new(&x->x_obj,&s_float);
    x->f_out3 = outlet_new(&x->x_obj,&s_float);
    inlet_new(&x->x_obj, &x->x_proxy->obj.ob_pd, 0, 0);
    return (x);
}

static void synlets_free(t_synlets *x) {
	pd_free((t_pd *)x->x_proxy);
}

void synlets_setup(void)
{
    synlets_class = class_new(gensym("synlets"), (t_newmethod)synlets_new,
        (t_method)synlets_free, sizeof(t_synlets), 0, A_GIMME, 0);
    class_addmethod(synlets_class, (t_method)synlets_set,
        gensym("set"), A_FLOAT, 0);
    class_addfloat(synlets_class, (t_method)synlets_float);
	class_addbang(synlets_class, (t_method)synlets_bang);

	proxy_class = class_new(gensym("shadylib_synlets_proxy"), NULL, NULL, sizeof(t_proxy),
		CLASS_PD|CLASS_NOINLET, A_NULL);
	class_addmethod(proxy_class, (t_method)proxy_set,
        gensym("set"), A_FLOAT, 0);
	class_addfloat(proxy_class, proxy_float);
	class_addbang(proxy_class, proxy_bang);
}
