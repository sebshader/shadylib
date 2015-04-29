#include "m_pd.h"
#include <stdlib.h>
#include <math.h>

static t_class *powclip_class, *scalarpowclip_class;

static t_sample *base_table;

#define LOGBASTABSIZE 11
#define BASTABSIZE (1<<LOGBASTABSIZE)
#define INC 1.0/BASTABSIZE

static void base_maketable(void)
{
	t_sample *fp;
	double funval = INC;
	if (base_table) return;
    base_table = (t_sample *)getbytes(sizeof(t_sample) * (BASTABSIZE+1));
    fp = base_table;
    *fp = 0;
    fp++;
    for (int i = BASTABSIZE - 1; i--; fp++, funval += INC)
            *fp = funval*(1 - log(funval));
    base_table[BASTABSIZE] = 1;
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

static void *powclip_new(t_symbol *s, int argc, t_atom *argv)
{
    if (argc > 1) post("powclip~: extra arguments ignored");
    if (argc) 
    {
        t_scalarpowclip *x = (t_scalarpowclip *)pd_new(scalarpowclip_class);
        floatinlet_new(&x->x_obj, &x->x_g);
        x->x_g = atom_getfloatarg(0, argc, argv);
        outlet_new(&x->x_obj, &s_signal);
        x->x_f = 0;
        return (x);
    }
    else
    {
        t_powclip *x = (t_powclip *)pd_new(powclip_class);
        inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
        outlet_new(&x->x_obj, &s_signal);
        x->x_f = 0;
        return (x);
    }
}

t_sample powclip_calculate (t_sample in1, t_sample in2) {
	t_sample absin1 = fabs(in1);
	if(absin1 > 1) absin1 = 1;
	if(in2 <= 0) return copysign(absin1, in1);
	else if(in2 >= 1) {
		int readpoint;
		double fred = absin1 * BASTABSIZE, val;
		readpoint = fred;
		fred -= readpoint;
		val = base_table[readpoint++];
	#ifdef FP_FAST_FMA
		return copysign(fma(fred, (base_table[readpoint] - val), val), in1);
	} else {
		return copysign(fma(-in2, pow(absin1, 1.0/in2), absin1)/(1 - in2), in1);
	} 
	#else
		return copysign(val + (base_table[readpoint] - val)*fred, in1);
	} else {
		return copysign((absin1 - in2*pow(absin1, 1.0/in2))/(1 - in2), in1);
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
    t_float f = *(t_float *)(w[2]);
    t_sample *out = (t_sample *)(w[3]);
    int n = (int)(w[4]);
    t_sample abstin, tin;
    if(f <= 0) while(n--) {
    	tin = *in++;
    	if(tin < -1) tin = -1;
    	else if (tin > 1) tin = 1;
    	*out++ = tin;
	} else if(f >= 1) {
		int readpoint;
		double fred, val;
		while (n--) {
			tin = *in++;
			abstin = fabs(tin);
			if(abstin > 1) abstin = 1;
			fred = abstin * BASTABSIZE;
			readpoint = fred;
			fred -= readpoint;
			val = base_table[readpoint++];
			#ifdef FP_FAST_FMA
			*out++ = copysign(fma(fred, (base_table[readpoint] - val),
				val), tin);
			#else
			*out++ = copysign(val + (base_table[readpoint] - val)*fred, tin);
			#endif
		}
	} else {
		double finverse = 1.0/f, fscale = 1/(1 - f);
		while (n--) {
			tin = *in++;
			abstin = fabs(tin);
			if(abstin > 1) abstin = 1;
			#ifdef FP_FAST_FMA
			*out++ = copysign(fma(-f, pow(abstin, finverse), abstin)*fscale,
				tin);
			#else
			*out++ = copysign((abstin - f*pow(abstin, finverse))*fscale, tin);
			#endif
		} 
	}
    return (w+5);
}

void dsp_add_powclip(t_sample *in1, t_sample *in2, t_sample *out, int n)
{
    dsp_add(powclip_perform, 4, in1, in2, out, n);
}

static void powclip_dsp(t_powclip *x, t_signal **sp)
{
    dsp_add_powclip(sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

static void scalarpowclip_dsp(t_scalarpowclip *x, t_signal **sp)
{
    dsp_add(scalarpowclip_perform, 4, sp[0]->s_vec, &x->x_g,
            sp[1]->s_vec, sp[0]->s_n);
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
	base_maketable();
}