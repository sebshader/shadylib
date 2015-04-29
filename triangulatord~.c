#include "shadylib.h"

static t_class *triangulatord_class;

typedef struct _triangulatord {
	t_object x_obj;
	t_float x_f;
	union floatpoint invals[2];
	int num; /* number of msg inlets: 0, 1, or 2 
		0 is all signals, 1 is an add inlet,
		2 is a multiply inlet and an add inlet */
} t_triangulatord;

static t_int *op_perf0(t_int *w) {
	t_triangulatord *x = (t_triangulatord *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_sample *add = x->invals[1].vec;
	t_sample inter;
	uint32_t casto;
    while (n--)
    {
        casto = (uint32_t)(*in++ * 4294967295);
        if(casto & 2147483648) /* bit 31 */
        	casto = ~casto;
        inter = (t_sample)casto/1073741823.5 - 1;
        #ifdef FP_FAST_FMA
        *out++ = fma(inter, *mul++, *add++);
        #else
        *out++ = inter*(*mul++) + (*add++);
        #endif
    }
    return (w+5);
}

static t_int *op_perf1(t_int *w) {
	t_triangulatord *x = (t_triangulatord *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_float add = x->invals[1].val;
	t_sample inter;
	uint32_t casto;
    while (n--)
    {
        casto = (uint32_t)(*in++ * 4294967295);
        if(casto & 2147483648) /* bit 31 */
        	casto = ~casto;
        inter = (t_sample)casto/1073741823.5 - 1;
        #ifdef FP_FAST_FMA
        *out++ = fma(inter, *mul++, add);
        #else
        *out++ = inter*(*mul++) + add;
        #endif
    }
    return (w+5);
}

static t_int *op_perf2(t_int *w) {
	t_triangulatord *x = (t_triangulatord *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_float mul = x->invals[0].val;
	t_float add = x->invals[1].val;
	t_sample inter;
	uint32_t casto;
    while (n--)
    {
        casto = (uint32_t)(*in++ * 4294967295);
        if(casto & 2147483648) /* bit 31 */
        	casto = ~casto;
        inter = (t_sample)casto/1073741823.5 - 1;
        #ifdef FP_FAST_FMA
        *out++ = fma(inter, mul, add);
        #else
        *out++ = inter*mul + add;
        #endif
    }
    return (w+5);
}

static void triangulatord_dsp(t_triangulatord *x, t_signal **sp) {
	switch(x->num) {
		case 0:
			x->invals[0].vec = sp[1]->s_vec;
			x->invals[1].vec = sp[2]->s_vec;
			dsp_add(op_perf0, 4, x, sp[0]->s_vec, sp[3]->s_vec, sp[0]->s_n);
			break;
		case 1:
			x->invals[0].vec = sp[1]->s_vec;
			dsp_add(op_perf1, 4, x, sp[0]->s_vec, sp[2]->s_vec, sp[0]->s_n);
			break;
		case 2:
			dsp_add(op_perf2, 4, x, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
			break;
		default:;
	}
}

static void *triangulatord_new(t_symbol *s, int argc, t_atom *argv) {
	t_triangulatord *x = (t_triangulatord *)pd_new(triangulatord_class);
	outlet_new(&x->x_obj, &s_signal);
	switch(argc) {
		default:;
		case 2:
			floatinlet_new(&x->x_obj, &x->invals[0].val);
			x->invals[0].val = atom_getfloatarg(0, argc, argv);
			floatinlet_new(&x->x_obj, &x->invals[1].val);
			x->invals[1].val = atom_getfloatarg(1, argc, argv);
			x->num = 2;
			break;
		case 1:
			inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
			floatinlet_new(&x->x_obj, &x->invals[1].val);
			x->invals[1].val = atom_getfloatarg(0, argc, argv);
			x->num = 1;
			break;
		case 0:
			inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
			inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
			x->num = 0;
	}
	return(x);
}

	

void triangulatord_tilde_setup(void)
{
    triangulatord_class = class_new(gensym("triangulatord~"), (t_newmethod)triangulatord_new, 0, sizeof(t_triangulatord), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(triangulatord_class, t_triangulatord, x_f);
    class_addmethod(triangulatord_class, (t_method)triangulatord_dsp, gensym("dsp"), 
    A_CANT, 0);
}
		
