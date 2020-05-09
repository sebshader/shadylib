#include "m_pd.h"
#include <math.h>
#define SINSQRSIZE 512

static t_sample *sinsqr_tbl; 

static t_class *vosim_class;

static void vosim_maketab(void) {
	t_sample incr = M_PI/(SINSQRSIZE - 1), val;
	sinsqr_tbl = getbytes((SINSQRSIZE) * sizeof(t_sample));
	for(int i = 0; i < SINSQRSIZE; i++) {
		val = sin(incr*i);
		sinsqr_tbl[i] = val*val;
	}
}

static void vosim_freetab(t_class *dummy) {
	freebytes(sinsqr_tbl, (SINSQRSIZE) * sizeof(t_sample));
}

static inline t_sample readtab(t_sample index) {
	/* it's the caller's responsibility to not create an overflow */
	int iindex = index;
	t_sample frac = index - iindex, index2;
	index = sinsqr_tbl[iindex++];
	index2 = sinsqr_tbl[iindex] - index;
	#ifdef FP_FAST_FMA
	return fma(frac, index2, index);
	#else
	return index + frac*index2;
	#endif
}

typedef struct _vosim {
	t_object x_obj;
    t_float x_f;
    double x_outphase; /* inner and outer frequency and phase */
    double x_inphase;
    float x_outfreq;
    float x_infreq;
    float x_res;
    float duty; /* duty cycle */
    float decay; /* decay coefficient */
    float curdec; /* current decay coefficient */
    int trans; /* we are transitioning phase */
    float x_conv; /* val to multiply freq by */
} t_vosim;

static t_int *vosim_perform(t_int *w) {
	t_vosim *x = (t_vosim *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	t_sample *cen = (t_sample *)(w[3]);
	t_sample *out = (t_sample *)(w[4]);
	int n = (int)(w[5]);
	int caster;
	t_sample phsinc;
	float decay = x->decay;
	float duty = x->duty;
	float curdec = x->curdec;
	float conv = x->x_conv;
	double inphase = x->x_inphase;
	double outphase = x->x_outphase, routphase;
	float infreq = x->x_infreq;
	float outfreq = x->x_outfreq;
	float res = x->x_res;

	for(int i = 0; i < n; i++) {
		if(outphase >= 1.) {
			curdec = 1;
			caster = outphase;
			outphase = outphase - caster;
			res = outphase;
			inphase = 0;
			phsinc = fmax(in[i], 0);
			outfreq = phsinc*conv;
			infreq = fmax(phsinc, cen[i])*conv;
		} else if (inphase >= 1.) {
			phsinc = fmax(in[i], 0);
			outfreq = phsinc*conv;
			infreq = fmax(phsinc, cen[i])*conv;
			caster = inphase;
			inphase = inphase - caster;
			/* routphase is now the limit */
			phsinc = outfreq/infreq;
			routphase = fmin(1 - phsinc, duty);
			routphase = routphase - outphase - res;
			if(routphase < 0) {
				curdec = 0;
			} else {
				curdec *= fmax(fmin(decay, 1.), -1.);
				/* a little fading */
				curdec *= fmin(routphase/phsinc, 1.);
			}
		 } else if (outfreq == 0 || curdec == 0 || outfreq == infreq) {
			phsinc = fmax(in[i], 0);
			outfreq = phsinc*conv;
			infreq = fmax(phsinc, cen[i])*conv;
		}
		out[i] = readtab(inphase*(SINSQRSIZE- 1))*curdec;
		outphase += outfreq;
		inphase += infreq;
	}
	x->x_res = res;
	x->curdec = curdec;
	x->x_inphase = inphase;
	x->x_outphase = outphase;
	x->x_infreq = infreq;
	x->x_outfreq = outfreq;
	return(w + 6);
}

static void vosim_dsp(t_vosim *x, t_signal **sp) {
	float oldconv = x->x_conv;
	float newconv = 1./sp[0]->s_sr;
	if(oldconv != newconv) {
		float oldofreq = x->x_outfreq, oldifreq = x->x_infreq;
		x->x_conv = newconv;
		x->x_outfreq = (oldofreq/oldconv)*newconv;
		x->x_infreq = (oldifreq/oldconv)*newconv;
	}
	dsp_add(vosim_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, 
		sp[0]->s_n);
}

static void vosim_reset(t_vosim *x)
{
    x->x_outphase = 0.0;
    x->x_inphase = 0.0;
    x->curdec = 1.0;
    x->x_infreq = 0;
    x->x_outfreq = 0;
    x->x_res = 0;
}

/* args: frequency, center freq., duty cycle, decay */
static void *vosim_new(t_symbol *s, int argc, t_atom *argv) {
	t_vosim *x = (t_vosim *)pd_new(vosim_class);
	x->x_f = 0.0;
	x->x_conv = 1.0;
	signalinlet_new(&x->x_obj, atom_getfloatarg(2, argc, argv));
    x->decay = atom_getfloatarg(0, argc, argv);
    if(argc > 1) x->duty = atom_getfloatarg(1, argc, argv);
    else x->duty = 1;
    vosim_reset(x);
	floatinlet_new(&x->x_obj, &x->decay);
	floatinlet_new(&x->x_obj, &x->duty);
	outlet_new(&x->x_obj, &s_signal);
	return(x);
}

void voisim_tilde_setup(void)
{
    vosim_class = class_new(gensym("voisim~"), 
        (t_newmethod)vosim_new, 0,
        sizeof(t_vosim), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(vosim_class, t_vosim, x_f);
    class_addmethod(vosim_class, (t_method)vosim_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(vosim_class, (t_method)vosim_reset,
    	gensym("reset"), 0);
    class_setfreefn(vosim_class, vosim_freetab);
    vosim_maketab();
}