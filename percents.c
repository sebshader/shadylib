#include "shadylib.h"

static t_class *percents_class;

#define m 16777216 /* 2^24: 24 digits are 0 */
#define m1 4096 /* 2^12 */

typedef struct _percents
{
    t_object x_obj;
    t_float *l_vec;  /* pointer to items */
    t_outlet *x_percentout;
    int l_n;            /* number of items */
    unsigned int x_state; /* current state (int) */
} t_percents;

// b = 4103221
static int percents_makeseed(void) {
    unsigned int p1, p0;
    static unsigned int nextseed = 13458715;

    p1 = nextseed/m1; p0 = nextseed%m1;
    nextseed = (((p0*1001 + p1*3125)&(m1 - 1))*m1 + p0*3125)&(m - 1);
    return nextseed;
}

static void percents_seed(t_percents *x, t_floatarg seed) {
    unsigned int modman = seed;
    x->x_state = modman&(m - 1);
}

static void percents_bang(t_percents *x)
{
    int bucket = 0;
    t_float rand;
    t_float *current;
    x->x_state = randlcm(x->x_state);
    rand = x->x_state*(1.0/167772.16);
    outlet_float(x->x_percentout, rand);
    for(current = x->l_vec; bucket < x->l_n; current++, bucket++) {
        if (rand < *current) {
            outlet_float(x->x_obj.ob_outlet, bucket);
            return;
        }
    }
    outlet_float(x->x_obj.ob_outlet, bucket);
}

static inline void percents_copyin(t_percents* x,
    t_symbol* SHADYLIB_UNUSED(s), int argc, t_atom *argv, int where)
{
    int i, j;
    for (i = 0, j = where; i < argc; i++)
    {
        if(argv[i].a_type == A_FLOAT) {
            x->l_vec[j] = argv[i].a_w.w_float;
            j++;
        }
    }
}

    /* set contents to a list */
static void percents_list(t_percents *x, t_symbol *s, int argc, t_atom *argv)
{
    if (x->l_vec)
        freebytes(x->l_vec, x->l_n * sizeof(*x->l_vec));
    int newargc = 0;
    /* count out floats */
    for(int i = 0; i < argc; i++) {
        if(argv[i].a_type == A_FLOAT) newargc++;
    }
    if (!(x->l_vec = (t_float *)getbytes(newargc *
        sizeof(*x->l_vec))))
    {
        x->l_n = 0;
        pd_error(0, "percents: out of memory");
        return;
    }
    x->l_n = newargc;
    percents_copyin(x, s, argc, argv, 0);
}

static void percents_free(t_percents *x)
{
    if (x->l_vec)
        freebytes(x->l_vec, x->l_n * sizeof(*x->l_vec));
}

static void *percents_new(t_symbol* SHADYLIB_UNUSED(s), int argc, t_atom *argv)
{
    t_percents *x = (t_percents *)pd_new(percents_class);
    outlet_new(&x->x_obj, &s_float);
    x->x_percentout = outlet_new(&x->x_obj, &s_float);
    x->l_n = 0;
    x->l_vec = 0;
    x->x_state = percents_makeseed();
    percents_list(x, 0, argc, argv);
    return (x);
}

void percents_setup(void)
{
    percents_class = class_new(gensym("percents"),
        (t_newmethod)percents_new, (t_method)percents_free,
        sizeof(t_percents), 0, A_GIMME, 0);
    class_addlist(percents_class, percents_list);
    class_addbang(percents_class, percents_bang);
    class_addmethod(percents_class, (t_method)percents_seed, gensym("seed"), A_DEFFLOAT, 0);
}
