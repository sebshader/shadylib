#include "m_pd.h"
#include <math.h>

#define MAXPRD 4194304

static t_class *highest_tilde_class;

typedef struct _highest
{
    t_object x_obj;
    t_outlet *x_outlet;                 /* a "float" outlet */
    void *x_clock;                  /* a "clock" object */
    t_sample x_result;
    t_float x_f;
    int x_period;
    int x_blocksize;
    int x_realperiod;
    int x_count;
} t_highest;

static void highest_set(t_highest *x, t_floatarg nsamps) {
	x->x_result = 0;
	x->x_count = 0;
	if(nsamps != 0.0) {
		int n = x->x_blocksize;
    	if(nsamps > MAXPRD) nsamps = MAXPRD;
    	x->x_period = nsamps;
    	if (x->x_period % n) x->x_realperiod =
        	x->x_period + n - (x->x_period % n);
    	else x->x_realperiod = x->x_period;
    }
}

static void highest_tilde_tick(t_highest *x) {
	outlet_float(x->x_outlet, x->x_result);
	x->x_result = 0;
}

static void highest_tilde_free(t_highest *x)           /* cleanup on free */
{
    clock_free(x->x_clock);
}

static void *highest_tilde_new(t_float nsamps) {
	t_highest *x = (t_highest *)pd_new(highest_tilde_class);
	x->x_outlet = outlet_new(&x->x_obj, &s_float);
    x->x_clock = clock_new(x, (t_method)highest_tilde_tick);
    x->x_blocksize = 0;
    if(nsamps < 1) x->x_period = 1024;
    else highest_set(x , nsamps);
    x->x_f = 0;
    return (void *)x;
}

static t_int *highest_tilde_perform(t_int *w) {
	t_highest *x = (t_highest *)(w[1]);
	t_sample *in = (t_sample *)(w[2]);
	int n = (int)(w[3]);
	t_sample result = x->x_result, temp;
	x->x_count += n;
	if(x->x_count >= x->x_realperiod) {
		x->x_count = 0;
		clock_delay(x->x_clock, 0L);
	}
	while(n--) {
		temp = fabs(*in);
		if(result < temp) result = temp;
		in++;
	}
	
	x->x_result = result;
	
	
	return (w+4);
}

static void highest_tilde_dsp(t_highest *x, t_signal **sp)
{
	int n = sp[0]->s_n;
	x->x_blocksize = n;
    if (x->x_period % n) x->x_realperiod =
        x->x_period + n - (x->x_period % n);
    else x->x_realperiod = x->x_period;
    x->x_count = 0;
    dsp_add(highest_tilde_perform, 3, x, sp[0]->s_vec, n);
}

void highest_tilde_setup(void)
{
	highest_tilde_class = class_new(gensym("highest~"), (t_newmethod)highest_tilde_new, (t_method)highest_tilde_free,
        sizeof(t_highest), CLASS_DEFAULT, A_DEFFLOAT, 0);
    CLASS_MAINSIGNALIN(highest_tilde_class, t_highest, x_f);
    class_addmethod(highest_tilde_class, (t_method)highest_tilde_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(highest_tilde_class, (t_method)highest_set,
        gensym("set"), A_FLOAT, 0);
    
}