#include "shadylib.h"

static t_class *triangulatord_class;

typedef struct _triangulatord {
    t_object x_obj;
    t_float x_f;
    t_shadylib_oscctl x_osc;
} t_triangulatord;

static void triangulatord_dsp(t_triangulatord *x, t_signal **sp) {
    switch(x->x_osc.num) {
        case 0:
            x->x_osc.invals[0].vec = sp[1]->s_vec;
            x->x_osc.invals[1].vec = sp[2]->s_vec;
            dsp_add(shadylib_trid_perf0, 4, &x->x_osc, sp[0]->s_vec, sp[3]->s_vec, sp[0]->s_n);
            break;
        case 1:
            x->x_osc.invals[0].vec = sp[1]->s_vec;
            dsp_add(shadylib_trid_perf1, 4, &x->x_osc, sp[0]->s_vec, sp[2]->s_vec, sp[0]->s_n);
            break;
        case 2:
            dsp_add(shadylib_trid_perf2, 4, &x->x_osc, sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
            break;
        default:;
    }
}

static void *triangulatord_new(t_symbol* SHADYLIB_UNUSED(s), int argc, t_atom *argv) {
    t_triangulatord *x = (t_triangulatord *)pd_new(triangulatord_class);
    outlet_new(&x->x_obj, &s_signal);
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
    return(x);
}



void triangulatord_tilde_setup(void)
{
    triangulatord_class = class_new(gensym("triangulatord~"), (t_newmethod)triangulatord_new, 0, sizeof(t_triangulatord), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(triangulatord_class, t_triangulatord, x_f);
    class_addmethod(triangulatord_class, (t_method)triangulatord_dsp, gensym("dsp"),
    A_CANT, 0);
}

