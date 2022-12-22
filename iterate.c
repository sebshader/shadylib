#include "m_pd.h"

static t_class *iterate_class;

typedef struct _iterate {
    t_object x_obj;
    int x_limit;
} t_iterate;

static void iterate_set(t_iterate *x, t_floatarg in) {
    x->x_limit = (in > 0) ? in : 0;
}

static void iterate_bang(t_iterate *x) {
    for(int i = 0; i < x->x_limit; i++)
        outlet_float(x->x_obj.ob_outlet, i);
}

static void iterate_float(t_iterate *x, t_floatarg in) {
    iterate_set(x, in);
    iterate_bang(x);
}

static void *iterate_new(t_floatarg in) {
    t_iterate *x = (t_iterate *)pd_new(iterate_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("set"));
    outlet_new(&x->x_obj, &s_float);
    iterate_set(x, in);
    return x;
}

void iterate_setup(void) {
    iterate_class = class_new(gensym("iterate"), (t_newmethod)iterate_new,
        0, sizeof(t_iterate), CLASS_DEFAULT, A_DEFFLOAT, 0);
    class_addbang(iterate_class, (t_method)iterate_bang);
    class_addfloat(iterate_class, (t_method)iterate_float);
    class_addmethod(iterate_class, (t_method)iterate_set, gensym("set"), A_DEFFLOAT, 0);
}
