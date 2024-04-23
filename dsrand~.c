#include "shadylib.h"

//linear congruential method from "Algorithms in C" by Robert Sedgewick
#define m 16777216 // 2^24: 24 digits are 0
#define m1 4096 // 2^12
#define b 2375621 /* arbitrary number ending in ...(even digit)21
                       with 1 digit less than m */

static t_class *dsrand_class;

typedef struct _dsrand {
    t_object x_obj;
    t_float x_f;
    float x_ahead; //second to last value generated
    float x_behind; //last value generated
    unsigned int x_state; //current "behind" state (int)
    t_sample x_lastin;
} t_dsrand;

// get a number from -1 to 1
static inline float ritoflt(unsigned int toflt) {
    return toflt*(1/8388607.5f) - 1.f;
}

static void dsrand_seed(t_dsrand *x, t_floatarg seed) {
    unsigned int modman = seed;
    x->x_state = modman%m;
}

static t_int *dsrand_perform(t_int *w) {
    t_dsrand  *x = (t_dsrand *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *out1 = (t_sample *)(w[3]);
    t_sample *out2 = (t_sample *)(w[4]);
    int n = (int)(w[5]);
    float ahead = x->x_ahead, behind = x->x_behind;
    t_sample lastin = x->x_lastin;
    unsigned int state = x->x_state;
    while(n--) {
        if(*in < lastin) {
            state = randlcm(state);
            ahead = behind;
            behind = ritoflt(state);
        }
        lastin = *in++;
        *out1++ = behind;
        *out2++ = ahead;
    }
    x->x_ahead = ahead; x->x_behind = behind;
    x->x_lastin = lastin;
    x->x_state = state;
    return w + 6;
}

static void dsrand_dsp(t_dsrand *x, t_signal **sp) {
    dsp_add(dsrand_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec,
            sp[2]->s_vec, sp[0]->s_n);
}

// b = 4103221
static int dsrand_makeseed(void) {
    unsigned int p1, p0;
    static unsigned int nextseed = 13458715;

    p1 = nextseed/m1; p0 = nextseed%m1;
    nextseed = (((p0*1001 + p1*3125)%m1)*m1 + p0*3125)%m;
    return nextseed;
}

static void *dsrand_new(t_symbol* SHADYLIB_UNUSED(s), int argc, t_atom *argv) {
    unsigned int state;
    t_dsrand *x = (t_dsrand *)pd_new(dsrand_class);

    if(argc)
        state = atom_getint(argv);
    else
        state = dsrand_makeseed();

    outlet_new(&x->x_obj, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    x->x_lastin = 0.f;
    x->x_ahead = 0.f;
    x->x_behind = 0.f;
    state %= m;
    x->x_state = state;
    return (x);
}

void dsrand_tilde_setup(void) {
    dsrand_class = class_new(gensym("dsrand~"), (t_newmethod)dsrand_new, 0,
        sizeof(t_dsrand), CLASS_DEFAULT, A_GIMME, 0);
    CLASS_MAINSIGNALIN(dsrand_class, t_dsrand, x_f);
    class_addmethod(dsrand_class, (t_method)dsrand_dsp, gensym("dsp"), A_CANT, 0);
    class_addmethod(dsrand_class, (t_method)dsrand_seed, gensym("seed"), A_DEFFLOAT, 0);
}
