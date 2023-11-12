#include "m_pd.h"

static t_class *mscaler_class;

typedef struct _scaler {
    t_object x_obj;
    t_float x_mul;
    t_float x_add;
} t_scaler;

static void scaler_float(t_scaler *x, t_floatarg in) {
    #ifdef FP_FAST_FMAF
    outlet_float(x->x_obj.ob_outlet,
        fmaf(x->x_mul, in, x->x_add));
    #else
    outlet_float(x->x_obj.ob_outlet, x->x_mul*in + x->x_add);
    #endif
}

static void *scaler_new(t_floatarg f1, t_floatarg f2)
{
    t_scaler *x = (t_scaler *)pd_new(mscaler_class);
    x->x_mul = f1;
    x->x_add = f2;
    floatinlet_new(&x->x_obj, &x->x_mul);
    floatinlet_new(&x->x_obj, &x->x_add);
    outlet_new(&x->x_obj, &s_float);
    return (x);
}

void scaler_setup(void)
{
    mscaler_class = class_new(gensym("scaler"), (t_newmethod)scaler_new,
        0, sizeof(t_scaler), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addfloat(mscaler_class, (t_method)scaler_float);
}