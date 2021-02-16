#include "m_pd.h"

static t_class *synlets_class;
static t_class *proxy_class;
#define MAX_INLET 256

typedef struct proxy
{
	t_object obj;
	struct _synlets *x;		/* we'll put the other struct in here */
	int x_count; /* count for inlet */
	int x_lim; /* limit for this inlet */
	int x_done; /* have we reached our limit? */
	t_outlet *f_out; /* outlet for count */
} t_proxy;

typedef struct _synlets {
	t_object x_obj;
    int x_donecount; /* # of inlets that are done */
	int x_count; /* count for leftmost inlet */
	int x_lim; /* limit for leftmost inlet */
	int x_done; /* have we reached our limit? */
	t_int x_ninstance; /* proxy instances for non-leftmost inlets */
    t_proxy **x_proxies;
	t_outlet *b_out, *f_out;
} t_synlets;

static void output(t_synlets *x) {
    t_proxy *y;
	x->x_donecount = 0;
    for(int i = x->x_ninstance - 1; i >= 0; i--)
	{
	    y = x->x_proxies[i];
	    y->x_count = 0;
	    y->x_done = 0;
	    if(!y->x_lim) x->x_donecount++; /* account for 0  as lim */
		outlet_float(y->f_out, 0.0);
	}
	x->x_count = 0;
	x->x_done = 0;
	outlet_float(x->f_out, 0.0);
	outlet_bang(x->b_out);
}

static void synlets_bang(t_synlets *x) {
	int count = x->x_count;
	count++;
	if(count >= x->x_lim && !x->x_done) {
		if(x->x_donecount >= x->x_ninstance) {
			output(x);
			return;
		} else {
		    x->x_done = 1;
		    if(x->x_lim) x->x_donecount++;
		}
	}
	x->x_count = count;
	outlet_float(x->f_out, count);
}

static void synlets_float(t_synlets *x, t_floatarg in) {
	int llim = in;
	if(llim <= x->x_count) {if(!x->x_done) {
        if (x->x_donecount >= x->x_ninstance) output(x);
        else {
            x->x_done = 1;
            if(x->x_lim) x->x_donecount++;
        }
	} } else if (x->x_done) {
	    x->x_done = 0;
	    x->x_donecount--;
	}
	x->x_lim = llim;
}

static void synlets_set(t_synlets *x, t_floatarg val) {
	int intval = val;
	int llim = x->x_lim;
	x->x_count = intval;
	if(llim <= intval) {if(!x->x_done) {
        x->x_done = 1;
        x->x_donecount++;
	} } else if (x->x_done) {
	    x->x_done = 0;
	    x->x_donecount--;
	}
}

static void proxy_bang(t_proxy *x) {
	int count = x->x_count;
	count++;
	if(count >= x->x_lim && !x->x_done) {
		if(x->x->x_donecount >= x->x->x_ninstance) {
			output(x->x);
			return;
		} else {
		    x->x_done = 1;
		    if(x->x_lim) x->x->x_donecount++;
		}
	}
	x->x_count = count;
	outlet_float(x->f_out, count);
}

static void proxy_float(t_proxy *x, t_floatarg in) {
	int llim = in;
	if(llim <= x->x_count) {if(!x->x_done) {
        if (x->x->x_donecount >= x->x->x_ninstance) output(x->x);
        else {
            x->x_done = 1;
            if(x->x_lim) x->x->x_donecount++;
        }
	} } else if (x->x_done) {
	    x->x_done = 0;
	    x->x->x_donecount--;
	}
	x->x_lim = llim;
}

static void proxy_set(t_proxy *x, t_floatarg val) {
	int intval = val;
	int llim = x->x_lim;
	x->x_count = intval;
	if(llim <= intval) {if(!x->x_done) {
        x->x_done = 1;
        x->x->x_donecount++;
	} } else if (x->x_done) {
	    x->x_done = 0;
	    x->x->x_donecount--;
	}
}

static void *synlets_new(t_symbol *s, int argc, t_atom *argv)
{
	t_proxy *iter;

    t_synlets *x = (t_synlets *)pd_new(synlets_class);
	x->b_out = outlet_new(&x->x_obj, &s_bang);
	x->x_count = 0;
	x->x_donecount = 0;
	x->x_done = 0;
	x->f_out = outlet_new(&x->x_obj, &s_float);
	if(argc < 2) {
		x->x_ninstance	= 1;
		x->x_proxies = getbytes(sizeof(t_proxy *));
		*(x->x_proxies) = (t_proxy *)pd_new(proxy_class);
		(*(x->x_proxies))->x_count = 0;
		(*(x->x_proxies))->x_done = 0;
		(*(x->x_proxies))->x_lim = 1;
		(*(x->x_proxies))->x = x;
		if(argc) {
		    x->x_lim = atom_getint(argv);
		} else x->x_lim = 1;
		if(!x->x_lim) x->x_donecount++;
		inlet_new(&x->x_obj, &(*(x->x_proxies))->obj.ob_pd, 0, 0);
		(*(x->x_proxies))->f_out = outlet_new(&x->x_obj, &s_float);
		return (void *)x;
	} else if(argc > MAX_INLET) argc = MAX_INLET;
	argc--;
	x->x_ninstance	= argc;
	x->x_proxies = getbytes(sizeof(t_proxy *) * argc);
	x->x_lim = atom_getint(argv);
	if(!x->x_lim) x->x_donecount++;
	for(int i = 0; i < argc; i++)
	{
		iter = (t_proxy *)pd_new(proxy_class);
		x->x_proxies[i] = iter;
		iter->x = x;		/* make x visible to the proxy inlets */
		iter->x_lim = atom_getint(argv+i+1);
		if(!iter->x_lim) x->x_donecount++;
		iter->x_count = 0;
		iter->x_done = 0;
			/* the inlet we're going to create belongs to the object 
		       't_prepender' but the destination is the instance 'i'
		       of the proxy class 't_proxy'                           */
		inlet_new(&x->x_obj, &iter->obj.ob_pd, 0, 0);
		iter->f_out = outlet_new(&x->x_obj, &s_float);
	}
    return (void *)x;
}

static void synlets_free(t_synlets *x)
{
	if(!x->x_ninstance) return;
	for(int i = 0; i < x->x_ninstance; i++)
	{
		pd_free((t_pd *)x->x_proxies[i]);
	}
	freebytes(x->x_proxies, sizeof(t_proxy *) * x->x_ninstance);
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
