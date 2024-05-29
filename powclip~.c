#include "shadylib.h"
#include <math.h>

static t_class *powclip_class, *scalarpowclip_class;

static t_sample *base_table;

#define LOGBASTABSIZE 10
#define BASTABSIZE (1<<LOGBASTABSIZE)
#define INC 1.0/(BASTABSIZE - 2)
/*
takes the antiderivative of the function 1-x^(f-1), with f
re-paramaterized as 1/input, so the antiderivative function is
(x-fx^(1/f))/(1-f) in order to keep it from 0 to 1.
when we use l'hopital's on this for the case f = 1 we get
x - xlnx
*/
static void base_maketable(void)
{
    t_sample *fp;
    double funval = INC;
    if (base_table) return;
    base_table = (t_sample *)getbytes(sizeof(t_sample) * (BASTABSIZE));
    fp = base_table;
    *fp = 0.f;
    fp++;
    for (int i = BASTABSIZE - 3; i--; fp++, funval += INC)
            *fp = funval*(1.0 - log(funval));
    base_table[BASTABSIZE - 2] = 1;
}

static void powclip_freebase(t_class* SHADYLIB_UNUSED(dummy)) {
    freebytes(base_table, sizeof(t_sample) * (BASTABSIZE));
}

typedef struct _powclip
{
    t_object x_obj;
    t_float x_f;
} t_powclip;

typedef struct _scalarpowclip
{
    t_object x_obj;
    t_float x_f;
    t_float x_g;            /* inlet value */
} t_scalarpowclip;

static void *powclip_new(t_symbol* SHADYLIB_UNUSED(s), int argc, t_atom *argv)
{
    if (argc > 1) post("powclip~: extra arguments ignored");
    if (argc)
    {
        t_scalarpowclip *x = (t_scalarpowclip *)pd_new(scalarpowclip_class);
        floatinlet_new(&x->x_obj, &x->x_g);
        x->x_g = atom_getfloatarg(0, argc, argv);
        outlet_new(&x->x_obj, &s_signal);
        x->x_f = 0.f;
        return (x);
    }
    else
    {
        t_powclip *x = (t_powclip *)pd_new(powclip_class);
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        outlet_new(&x->x_obj, &s_signal);
        x->x_f = 0.f;
        return (x);
    }
}

static inline t_sample powclip_calculate (t_sample in1, double in2) {
    t_sample absin1 = shadylib_pdfloat_abs(in1);
    absin1 = shadylib_min(absin1, 1.f);
    if(in2 <= 0.0) return shadylib_copysign(absin1, in1);
    else if(in2 >= 1.0) {
        int readpoint;
        t_sample fred = absin1 * (BASTABSIZE - 2), val;
        readpoint = fred;
        fred -= readpoint;
        val = base_table[readpoint++];
    #ifdef FP_FAST_FMAF
        return shadylib_copysign(fmaf(fred, (base_table[readpoint] - val), val), in1);
    } else {
        return shadylib_copysign(fmaf(-in2, pow(absin1, 1.0/in2), absin1)/(1 - in2), in1);
    }
    #else
        return shadylib_copysign(val + (base_table[readpoint] - val)*fred, in1);
    } else {
        return shadylib_copysign((absin1 - in2*pow(absin1, 1.0/in2))/(1. - in2), in1);
    }
    #endif
}

t_int *powclip_perform(t_int *w)
{
    t_sample *in1 = (t_sample *)(w[1]);
    t_sample *in2 = (t_sample *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    while (n--) {
        *out++ = powclip_calculate(*in1++, *in2++);
    }
    return (w+5);
}

t_int *scalarpowclip_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    double f = *(t_float *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample abstin, tin;
    if(f <= 0.f) while(n--) {
        tin = *in++;
        *out++ = shadylib_clamp(tin, -1.f, 1.f);
    } else if(f >= 1.0) {
        int readpoint;
        t_sample fred, val;
        while (n--) {
            tin = *in++;
            abstin = shadylib_min(shadylib_pdfloat_abs(tin), 1.f);
            fred = abstin * (BASTABSIZE - 2);
            readpoint = fred;
            fred -= readpoint;
            val = base_table[readpoint++];
            #ifdef FP_FAST_FMAF
            *out++ = shadylib_copysign(fmaf(fred, (base_table[readpoint] - val),
                val), tin);
            #else
            *out++ = shadylib_copysign(val + (base_table[readpoint] - val)*fred, tin);
            #endif
        }
    } else {
        double finverse = 1.0/f;
        while (n--) {
            tin = *in++;
            abstin = shadylib_min(shadylib_pdfloat_abs(tin), 1.f);
            #ifdef FP_FAST_FMAF
            *out++ = shadylib_copysign(fmaf(-f, pow(abstin, finverse), abstin)/(1.0 - f),
                tin);
            #else
            *out++ = shadylib_copysign((abstin - f*pow(abstin, finverse))/(1.0 - f), tin);
            #endif
        }
    }
    return (w+5);
}

static void powclip_dsp(t_powclip* SHADYLIB_UNUSED(x), t_signal **sp)
{
    dsp_add(powclip_perform, 4, sp[0]->s_vec, sp[1]->s_vec,
        sp[2]->s_vec, sp[0]->s_n);
}

static void scalarpowclip_dsp(t_scalarpowclip *x, t_signal **sp)
{
    dsp_add(scalarpowclip_perform, 4, sp[0]->s_vec, &x->x_g, sp[1]->s_vec,
        sp[0]->s_n);
}

void powclip_tilde_setup(void)
{
    powclip_class = class_new(gensym("powclip~"), (t_newmethod)powclip_new, 0,
        sizeof(t_powclip), 0, A_GIMME, 0);
    class_addmethod(powclip_class, (t_method)powclip_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(powclip_class, t_powclip, x_f);
    scalarpowclip_class = class_new(gensym("powclip~"), 0, 0,
        sizeof(t_scalarpowclip), 0, 0);
    CLASS_MAINSIGNALIN(scalarpowclip_class, t_scalarpowclip, x_f);
    class_addmethod(scalarpowclip_class, (t_method)scalarpowclip_dsp,
        gensym("dsp"), A_CANT, 0);
    class_setfreefn(powclip_class, powclip_freebase);
    base_maketable();
}
