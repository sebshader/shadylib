#include "shadylib.h"

static t_class *rectoratord_class;

typedef struct _rectoratord {
	t_object x_obj;
	t_float x_f;
	union floatpoint invals[2];
	int num; /* number of msg inlets: 0, 1, or 2 
		0 is all signals, 1 is an add inlet,
		2 is a multiply inlet and an add inlet */
} t_rectoratord;

static t_int *op_perf0(t_int *w) {
	t_rectoratord *x = (t_rectoratord *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_sample *add = x->invals[1].vec;
    union tabfudge inter;
	uint32_t casto;
	inter.tf_d = 1.0;
    while (n--)
    {
        casto = (uint32_t)(*in++ * 4294967295);
        /* set the sign bit of double 1.0 */
        inter.tf_i[HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        #ifdef FP_FAST_FMA
        *out++ = fma(inter.tf_d, *mul++, *add++);
        #else
        *out++ = inter.tf_d*(*mul++) + (*add++);
        #endif
    }
    return (w+5);
}

static t_int *op_perf1(t_int *w) {
	t_rectoratord *x = (t_rectoratord *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_sample *mul = x->invals[0].vec;
	t_float add = x->invals[1].val;
	union tabfudge inter;
	uint32_t casto;
	inter.tf_d = 1.0;
    while (n--)
    {
        casto = (uint32_t)(*in++ * 4294967295);
        inter.tf_i[HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        #ifdef FP_FAST_FMA
        *out++ = fma(inter.tf_d, *mul++, add);
        #else
        *out++ = inter.tf_d*(*mul++) + add;
        #endif
    }
    return (w+5);
}

static t_int *op_perf2(t_int *w) {
	t_rectoratord *x = (t_rectoratord *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *out = (t_sample *)(w[3]);
	int n = (int)(w[4]);
	t_float mul = x->invals[0].val;
	t_float add = x->invals[1].val;
	union tabfudge inter;
	uint32_t casto;
	inter.tf_d = 1.0;
    while (n--)
    {
        casto = (uint32_t)(*in++ * 4294967295);
        inter.tf_i[HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        #ifdef FP_FAST_FMA
        *out++ = fma(inter.tf_d, mul, add);
        #else
        *out++ = inter.tf_d*mul + add;
        #endif
    }
    return (w+5);
}

static void rectoratord_dsp(t_rectoratord *x, t_signal **sp) {
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

static void *rectoratord_new(t_symbol *s, int argc, t_atom *argv) {
	t_rectoratord *x = (t_rectoratord *)pd_new(rectoratord_class);
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

	

void rectoratord_tilde_setup(void)
{
    rectoratord_class = class_new(gensym("rectoratord~"), (t_newmethod)rectoratord_new, 0, sizeof(t_rectoratord), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(rectoratord_class, t_rectoratord, x_f);
    class_addmethod(rectoratord_class, (t_method)rectoratord_dsp, gensym("dsp"), 
    A_CANT, 0);
}
		
