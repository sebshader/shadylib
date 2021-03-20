#include "m_pd.h"

static t_class *array_redraw_class;

typedef struct _array_redraw
{
    t_object x_obj;
    t_symbol *x_arrayname;
} t_array_redraw;

static void array_redraw_bang(t_array_redraw *x) {
    t_garray *a;

    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
        pd_error(x, "%s: no such array", x->x_arrayname->s_name);
    else garray_redraw(a);
}

static void array_redraw_set(t_array_redraw *x, t_symbol *s)
{
    x->x_arrayname = s;
}

static void *array_redraw_new(t_symbol *s)
{
    t_array_redraw *x = (t_array_redraw *)pd_new(array_redraw_class);
    x->x_arrayname = s;
    return (x);
}

void setup_array0x2dredraw(void)
{
    array_redraw_class = class_new(gensym("array-redraw"),
        (t_newmethod)array_redraw_new, 0,
        sizeof(t_array_redraw), 0, A_DEFSYM, 0);
    class_addbang(array_redraw_class, array_redraw_bang);
    class_addmethod(array_redraw_class, (t_method)array_redraw_set,
        gensym("set"), A_SYMBOL, 0);
}