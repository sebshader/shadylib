#include "shadylib.h"

static t_class *syphon_class;

typedef struct _syphon
{
    t_object x_obj;
    t_shadylib_alist x_alist;
    t_outlet *x_rejectout;
} t_syphon;

static void *syphon_new(t_symbol* UNUSED(s), int argc, t_atom *argv)
{
    t_syphon *x = (t_syphon *)pd_new(syphon_class);
    shadylib_alist_init(&x->x_alist);
    shadylib_alist_list(&x->x_alist, 0, argc, argv);
    outlet_new(&x->x_obj, NULL);
    x->x_rejectout = outlet_new(&x->x_obj, NULL);
    inlet_new(&x->x_obj, &x->x_alist.l_pd, 0, 0);
    return (x);
}

static void syphon_bang(t_syphon *x)
{
    int n;
    t_shadylib_listelem *current;
    for(n = x->x_alist.l_n, current = x->x_alist.l_vec; n--; current++) {
        if(current->l_a.a_type == A_SYMBOL
            && &s_bang == current->l_a.a_w.w_symbol) {
            outlet_bang(x->x_obj.ob_outlet);
            return;
        }
    }
    outlet_bang(x->x_rejectout);
}

static void syphon_list(t_syphon *x, t_symbol *s,
    int argc, t_atom *argv)
{
    int n;
    t_shadylib_listelem *current;
    if(argc)
        for(n = x->x_alist.l_n, current = x->x_alist.l_vec; n--; current++) {
            if(shadylib_atoms_eq(argv, &current->l_a)) {
                outlet_list(x->x_obj.ob_outlet, s, argc, argv);
                return;
            }
        }
    outlet_list(x->x_rejectout, s, argc, argv);
}

static void syphon_symbol(t_syphon *x, t_symbol *s)
{
    int n;
    t_shadylib_listelem *current;
    for(n = x->x_alist.l_n, current = x->x_alist.l_vec; n--; current++) {
        if(current->l_a.a_type == A_SYMBOL
            && s == current->l_a.a_w.w_symbol) {
            outlet_symbol(x->x_obj.ob_outlet, s);
            return;
        }
    }
    outlet_symbol(x->x_rejectout, s);
}

static void syphon_float(t_syphon *x, t_float f)
{
    int n;
    t_shadylib_listelem *current;
    for(n = x->x_alist.l_n, current = x->x_alist.l_vec; n--; current++) {
        if(current->l_a.a_type == A_FLOAT && f == current->l_a.a_w.w_float) {
            outlet_float(x->x_obj.ob_outlet, f);
            return;
        }
    }
    outlet_float(x->x_rejectout, f);
}

static void syphon_anything(t_syphon *x, t_symbol *s,
    int argc, t_atom *argv)
{
    int n;
    t_shadylib_listelem *current;
    for(n = x->x_alist.l_n, current = x->x_alist.l_vec; n--; current++) {
        if(current->l_a.a_type == A_SYMBOL
            && s == current->l_a.a_w.w_symbol) {
            outlet_anything(x->x_obj.ob_outlet, s, argc, argv);
            return;
        }
    }
    outlet_anything(x->x_rejectout, s, argc, argv);
}

static void syphon_free(t_syphon *x)
{
    shadylib_alist_clear(&x->x_alist);
}

void syphon_setup(void)
{
    if(!shadylib_alist_class) shadylib_alist_setup();
    syphon_class = class_new(gensym("syphon"),
        (t_newmethod)syphon_new, (t_method)syphon_free,
        sizeof(t_syphon), 0, A_GIMME, 0);
    class_addlist(syphon_class, syphon_list);
    class_addsymbol(syphon_class, syphon_symbol);
    class_addfloat(syphon_class, syphon_float);
    class_addanything(syphon_class, syphon_anything);
    class_addbang(syphon_class, syphon_bang);
}
