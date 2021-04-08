#include "shadylib.h"

static t_class *siglinterp_class, *scalarsiglinterp_class;

typedef struct _siglinterp
{
    t_object x_obj;
    t_float x_f;
    
	int n_in;
	t_sample **in;

} t_siglinterp;

typedef struct _scalarsiglinterp
{
    t_object x_obj;
    t_float x_f;
    
	t_float input;
	int n_in;
	t_sample **in;

    t_float x_g;            /* inlet value */
} t_scalarsiglinterp;

static void *siglinterp_new(t_symbol* UNUSED(s), int argc, t_atom *argv)
{
	int nsigs;
	if(argc) {
		nsigs = atom_getfloatarg(0, argc, argv);
		if(nsigs < 2) nsigs = 2;
	} else {
		nsigs = 2;
	}
	if (argc >= 2) {
        t_scalarsiglinterp *x = 
            (t_scalarsiglinterp *)pd_new(scalarsiglinterp_class);
        x->n_in = nsigs--;
        while(nsigs--)
        	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        floatinlet_new(&x->x_obj, &x->x_g);
        x->x_g = atom_getfloatarg(1, argc, argv);
        x->in = (t_sample **)getbytes(x->n_in * sizeof(t_sample *));
		outlet_new(&x->x_obj, &s_signal);
		x->x_f = 0;
		return (x);
    }
    else
    {
        t_siglinterp *x = (t_siglinterp *)pd_new(siglinterp_class);
        x->n_in = nsigs--;
        while(nsigs--)
        	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        x->in = (t_sample **)getbytes(x->n_in * sizeof(t_sample *));
		outlet_new(&x->x_obj, &s_signal);
		x->x_f = 0;
		return (x);
    }
}

static void siglinterp_free(t_siglinterp *x)
{
	freebytes(x->in, x->n_in * sizeof(t_sample *));
}

static void scalarsiglinterp_free(t_scalarsiglinterp *x)
{
	freebytes(x->in, x->n_in * sizeof(t_sample *));
}

t_int *scalarsiglinterp_perform(t_int *w)
{
    t_scalarsiglinterp *x = (t_scalarsiglinterp *)(w[1]);
    t_float findex = *(t_float *)(w[2]), frac;
    t_sample *out = (t_sample *)(w[3]), *in1, *in2;
    int n = (int)(w[4]);
    int index = findex;
    if (index < 0)
		index = 0, frac = 0;
	else if (index > x->n_in - 2)
		index = x->n_in - 2, frac = 1;
    else frac = findex - index;
    in1 = x->in[index];
    in2 = x->in[index+1];
    while (n--)
    	*out++ = ((*in2++) * frac) + *in1++*(1-frac);
    return (w+5);
}

t_int *scalarsiglinterp_perf8(t_int *w)
{
    t_scalarsiglinterp *x = (t_scalarsiglinterp *)(w[1]);
    t_float findex = *(t_float *)(w[2]), frac;
    t_sample *out = (t_sample *)(w[3]), *in1, *in2;
    int n = (int)(w[4]);
    int index = findex;
    if (index < 0)
		index = 0, frac = 0;
	else if (index > x->n_in - 2)
		index = x->n_in - 2, frac = 1;
    else frac = findex - index;
    in1 = x->in[index];
    in2 = x->in[index+1];
    for (; n; n -= 8, in1 += 8, in2 += 8, out += 8)
    {
        out[0] = ((in2[0]) * frac) + in1[0]*(1-frac);
        out[1] = ((in2[1]) * frac) + in1[1]*(1-frac);
        out[2] = ((in2[2]) * frac) + in1[2]*(1-frac);
        out[3] = ((in2[3]) * frac) + in1[3]*(1-frac);
        out[4] = ((in2[4]) * frac) + in1[4]*(1-frac);
        out[5] = ((in2[5]) * frac) + in1[5]*(1-frac);
        out[6] = ((in2[6]) * frac) + in1[6]*(1-frac);
        out[7] = ((in2[7]) * frac) + in1[7]*(1-frac);
    }
    return (w+5);
}

t_int *siglinterp_perform(t_int *w)
{
    t_siglinterp *x = (t_siglinterp *)(w[1]);
    t_sample *findex = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]), *in1, *in2;
    t_float frac;
    int n = (int)(w[4]), index;
    for (int i = 0; i < n; i++) {
    	index = *findex;
		if (index < 0)
			index = 0, frac = 0;
		else if (index > x->n_in - 2)
			index = x->n_in - 2, frac = 1;
		else frac = *findex - index;
		findex++;
		in1 = x->in[index++];
		in2 = x->in[index];
    	*out++ = in2[i]*frac + in1[i]*(1-frac);
    }
    return (w+5);
}

static void scalarsiglinterp_dsp(t_scalarsiglinterp *x, t_signal **sp)
{
	for (int i = 0; i < x->n_in; i++) x->in[i] = sp[i]->s_vec;

	if (sp[0]->s_n & 7)
    	dsp_add(scalarsiglinterp_perform, 4, x, &x->x_g, sp[x->n_in]->s_vec,
    		sp[0]->s_n);
    else
    	dsp_add(scalarsiglinterp_perf8, 4, x, &x->x_g, sp[x->n_in]->s_vec,
    		sp[0]->s_n);
}

static void siglinterp_dsp(t_siglinterp *x, t_signal **sp)
{
	for (int i = 0; i < x->n_in; i++) x->in[i] = sp[i]->s_vec;

	dsp_add(siglinterp_perform, 4, x, sp[x->n_in]->s_vec, 
		sp[x->n_in+1]->s_vec, sp[0]->s_n);
}

void siglinterp_tilde_setup(void)
{
    siglinterp_class = class_new(gensym("siglinterp~"), 
    	(t_newmethod)siglinterp_new, (t_method)siglinterp_free,
        sizeof(t_siglinterp), 0, A_GIMME, 0);
    class_addmethod(siglinterp_class, (t_method)siglinterp_dsp, 
    	gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(siglinterp_class, t_siglinterp, x_f);
    scalarsiglinterp_class = class_new(gensym("siglinterp~"), 0,
    	(t_method)scalarsiglinterp_free, sizeof(t_scalarsiglinterp), 0, 0);
    CLASS_MAINSIGNALIN(scalarsiglinterp_class, t_scalarsiglinterp, x_f);
    class_addmethod(scalarsiglinterp_class, (t_method)scalarsiglinterp_dsp,
        gensym("dsp"), A_CANT, 0);
}