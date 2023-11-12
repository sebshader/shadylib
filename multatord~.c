#include "shadylib.h"

static t_class *multatord_class;

typedef struct _multatord {
    t_object x_obj;
    t_float x_f;
    t_shadylib_oscctl x_osc;
    float rwave;
} t_multatord;

/* this is just wrap~ with multiply*/
static t_int *multator_saw_perf0(t_int *w)
{
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *mul = x->invals[0].vec;
    t_sample *add = x->invals[1].vec;
    t_sample f;
    int k;
    while (n--)
    {
        f = *in++;
        k = f;
        #ifdef FP_FAST_FMAF
        if (k <= f) f = fmaf(f-k, 2, -1);
        else f = fmaf(f-k+1, 2, -1);
        *out++ = fmaf(f, *mul++, *add++);
        #else
        if (k <= f) f = (f-k)*2 - 1;
        else f = (f-k+1)*2 - 1;
        *out++ = f*(*mul++) + (*add++);
        #endif
    }
    return (w+5);
}

static t_int *multator_saw_perf1(t_int *w)
{
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample *mul = x->invals[0].vec;
    t_float add = x->invals[1].val;
    t_sample f;
    int k;
    while (n--)
    {
        f = *in++;
        k = f;
        #ifdef FP_FAST_FMAF
        if (k <= f) f = fmaf(f-k, 2, -1);
        else f = fmaf(f-k+1, 2, -1);
        *out++ = fmaf(f, *mul++, add);
        #else
        if (k <= f) f = (f-k)*2 - 1;
        else f = (f-k+1)*2 - 1;
        *out++ = f*(*mul++) + add;
        #endif
    }
    return (w+5);
}

static t_int *multator_saw_perf2(t_int *w)
{
    t_shadylib_oscctl *x = (t_shadylib_oscctl *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_float mul = x->invals[0].val;
    t_float add = x->invals[1].val;
    t_sample f;
    int k;
    while (n--)
    {
        f = *in++;
        k = f;
        #ifdef FP_FAST_FMAF
        if (k <= f) f = fmaf(f-k, 2, -1);
        else f = fmaf(f-k+1, 2, -1);
        *out++ = fmaf(f, mul, add);
        #else
        if (k <= f) f = (f-k)*2 - 1;
        else f = (f-k+1)*2 - 1;
        *out++ = f*mul + add;
        #endif
    }
    return (w+5);
}

static t_int *mul_perf0(t_int *w) {
    int rwave = (int)*(float *)(w[1]);
    switch (rwave) {
        default:;
        case 0:
            return shadylib_opd_perf0(w + 1);
        case 1:
            return shadylib_recd_perf0(w + 1);
        case 2:
            return shadylib_trid_perf0(w + 1);
        case 3:
            return multator_saw_perf0(w + 1);
    }
}

static t_int *mul_perf1(t_int *w) {
    int rwave = (int)*(float *)(w[1]);
    switch (rwave) {
        default:;
        case 0:
            return shadylib_opd_perf1(w + 1);
        case 1:
            return shadylib_recd_perf1(w + 1);
        case 2:
            return shadylib_trid_perf1(w + 1);
        case 3:
            return multator_saw_perf1(w + 1);
    }
}

static t_int *mul_perf2(t_int *w) {
    int rwave = (int)*(float *)(w[1]);
    switch (rwave) {
        default:;
        case 0:
            return shadylib_opd_perf2(w + 1);
        case 1:
            return shadylib_recd_perf2(w + 1);
        case 2:
            return shadylib_trid_perf2(w + 1);
        case 3:
            return multator_saw_perf2(w + 1);
    }
}

static void multatord_dsp(t_multatord *x, t_signal **sp) {
    switch(x->x_osc.num) {
        case 0:
            x->x_osc.invals[0].vec = sp[1]->s_vec;
            x->x_osc.invals[1].vec = sp[2]->s_vec;
            dsp_add(mul_perf0, 5, &x->rwave, &x->x_osc, sp[0]->s_vec, sp[3]->s_vec, sp[0]->s_n);
            break;
        case 1:
            x->x_osc.invals[0].vec = sp[1]->s_vec;
            dsp_add(mul_perf1, 5, &x->rwave, &x->x_osc, sp[0]->s_vec, sp[2]->s_vec, sp[0]->s_n);
            break;
        case 2:
            dsp_add(mul_perf2, 5, &x->rwave, &x->x_osc, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
            break;
        default:;
    }
}

static void *multatord_new(t_symbol* UNUSED(s), int argc, t_atom *argv) {
    t_multatord *x = (t_multatord *)pd_new(multatord_class);
    outlet_new(&x->x_obj, &s_signal);
    x->rwave = (argc >= 3) ? atom_getfloatarg(2, argc, argv) : 0;
    switch(argc) {
        default:;
        case 2:
            floatinlet_new(&x->x_obj, &x->x_osc.invals[0].val);
            x->x_osc.invals[0].val = atom_getfloatarg(0, argc, argv);
            floatinlet_new(&x->x_obj, &x->x_osc.invals[1].val);
            x->x_osc.invals[1].val = atom_getfloatarg(1, argc, argv);
            x->x_osc.num = 2;
            break;
        case 1:
            inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
            floatinlet_new(&x->x_obj, &x->x_osc.invals[1].val);
            x->x_osc.invals[1].val = atom_getfloatarg(0, argc, argv);
            x->x_osc.num = 1;
            break;
        case 0:
            inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
            inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
            x->x_osc.num = 0;
    }
    floatinlet_new(&x->x_obj, &x->rwave);
    return(x);
}

void multatord_tilde_setup(void)
{
    multatord_class = class_new(gensym("multatord~"), (t_newmethod)multatord_new, 0, sizeof(t_multatord), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(multatord_class, t_multatord, x_f);
    class_addmethod(multatord_class, (t_method)multatord_dsp, gensym("dsp"),
    A_CANT, 0);
    shadylib_checkalign();
    class_setfreefn(multatord_class, shadylib_freebuzz);
    shadylib_makebuzz();
}

