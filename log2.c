#include <math.h>
#include "m_pd.h"

static t_class *log2_class;

typedef struct _log2 {
    t_object x_obj;
} t_log2;

static void log2_float(t_log2 *x, t_floatarg in) {
    outlet_float(x->x_obj.ob_outlet, log2(in));
}

static void *log2_new(void)
{
    t_log2 *x = (t_log2 *)pd_new(log2_class);
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

void log2_setup(void)
{
    log2_class = class_new(gensym("log2"), (t_newmethod)log2_new,
        0, sizeof(t_log2), 0, 0);
    class_addfloat(log2_class, (t_method)log2_float);
}