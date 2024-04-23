#include "shadylib.h"

static t_class *rectorator_class;

typedef struct _rectorator {
    t_object x_obj;
    t_float x_f;
    union shadylib_floatpoint invals[2];
    int num; /* number of msg inlets: 0, 1, or 2
        0 is all signals, 1 is an add inlet,
        2 is a multiply inlet and an add inlet */
    double x_phase;
    float x_conv;
} t_rectorator;

static t_int *op_perf0(t_int *w) {
    t_rectorator *x = (t_rectorator *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *mul = x->invals[0].vec;
    t_sample *add = x->invals[1].vec;
    double dphase = x->x_phase + (double)SHADYLIB_UNITBIT32;
    union shadylib_tabfudge tf;
    union shadylib_tabfudge inter;
    uint32_t casto;

    float conv = x->x_conv;
    tf.tf_d = dphase;
    inter.tf_d = 1.0;
    while (n--)
    {
        dphase += *in++ * conv;
        casto = (uint32_t)tf.tf_i[SHADYLIB_LOWOFFSET];
        inter.tf_i[SHADYLIB_HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        *out++ = inter.tf_d*(*mul++) + (*add++);
        tf.tf_d = dphase;
    }
    tf.tf_i[SHADYLIB_HIOFFSET] = SHADYLIB_NORMHIPART;
    x->x_phase = tf.tf_d - SHADYLIB_UNITBIT32;
    return (w+5);
}

static t_int *op_perf1(t_int *w) {
    t_rectorator *x = (t_rectorator *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *mul = x->invals[0].vec;
    t_float add = x->invals[1].val;
    double dphase = x->x_phase + (double)SHADYLIB_UNITBIT32;
    union shadylib_tabfudge tf;
    union shadylib_tabfudge inter;
    uint32_t casto;

    float conv = x->x_conv;
    tf.tf_d = dphase;
    inter.tf_d = 1.0;
    while (n--)
    {
        dphase += *in++ * conv;
        casto = (uint32_t)tf.tf_i[SHADYLIB_LOWOFFSET];
        inter.tf_i[SHADYLIB_HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        *out++ = inter.tf_d*(*mul++) + add;
        tf.tf_d = dphase;
    }
    tf.tf_i[SHADYLIB_HIOFFSET] = SHADYLIB_NORMHIPART;
    x->x_phase = tf.tf_d - SHADYLIB_UNITBIT32;
    return (w+5);
}

static t_int *op_perf2(t_int *w) {
    t_rectorator *x = (t_rectorator *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_float mul = x->invals[0].val;
    t_float add = x->invals[1].val;
    double dphase = x->x_phase + (double)SHADYLIB_UNITBIT32;
    union shadylib_tabfudge tf;
    union shadylib_tabfudge inter;
    uint32_t casto;

    float conv = x->x_conv;
    tf.tf_d = dphase;
    inter.tf_d = 1.0;
    while (n--)
    {
        dphase += *in++ * conv;
        casto = (uint32_t)tf.tf_i[SHADYLIB_LOWOFFSET];
        inter.tf_i[SHADYLIB_HIOFFSET] = 1072693248 | (casto & 2147483648); /* bit 31 */
        *out++ = inter.tf_d*mul + add;
        tf.tf_d = dphase;
    }
    tf.tf_i[SHADYLIB_HIOFFSET] = SHADYLIB_NORMHIPART;
    x->x_phase = tf.tf_d - SHADYLIB_UNITBIT32;
    return (w+5);
}

static void rectorator_dsp(t_rectorator *x, t_signal **sp) {
    x->x_conv = 1/sp[0]->s_sr;
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

static void rectorator_ft1(t_rectorator *x, t_float f)
{
    x->x_phase = f;
}

static void *rectorator_new(t_symbol* SHADYLIB_UNUSED(s), int argc, t_atom *argv) {
    t_rectorator *x = (t_rectorator *)pd_new(rectorator_class);
    x->x_phase = 0;
    outlet_new(&x->x_obj, &s_signal);
    switch(argc) {
        default:;
        case 3:
            x->x_f = atom_getfloatarg(2, argc, argv);
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
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("ft1"));
    return(x);
}



void rectorator_tilde_setup(void)
{
    rectorator_class = class_new(gensym("rectorator~"), (t_newmethod)rectorator_new, 0, sizeof(t_rectorator), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(rectorator_class, t_rectorator, x_f);
    class_addmethod(rectorator_class, (t_method)rectorator_dsp, gensym("dsp"),
    A_CANT, 0);
    class_addmethod(rectorator_class, (t_method)rectorator_ft1,
        gensym("ft1"), A_FLOAT, 0);
    shadylib_checkalign();
}

