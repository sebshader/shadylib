#include "m_pd.h"

EXTERN void parsetimeunits(void *x, t_float amount, t_symbol *unitname,
    t_float *unit, int *samps);

static t_class *upmet_class;

typedef struct _upmet
{
    t_object x_obj;
    t_clock *x_clock;
    double x_settime;
/* guess we have to store the units too? */
    t_float x_unit;
    int x_samps;
    
    double x_deltime;
    
/* to hold delay time that we are using until the next tick */
	double x_ttime;
    int x_hit;
    int x_onoff;
} t_upmet;

static void upmet_ft1(t_upmet *x, t_floatarg g)
{
	if (g <= 0) /* as of 0.45, we're willing to try any positive time value */
        g = 1;  /* but default to 1 (arbitrary and probably not so good) */

    if (x->x_onoff) {
		double olddel = x->x_ttime;
		double deltime = x->x_deltime;
		double time = clock_gettimesincewithunits(x->x_settime,
			x->x_unit, x->x_samps);
		deltime = g*(olddel - time)/deltime;
    	clock_delay(x->x_clock, deltime);
    	x->x_ttime = time + deltime;
    }
    x->x_deltime = g;
}

static void upmet_tick(t_upmet *x)
{
    x->x_hit = 0;
    outlet_bang(x->x_obj.ob_outlet);
    if (!x->x_hit) {
    	clock_delay(x->x_clock, x->x_deltime);
    	x->x_ttime = x->x_deltime;
    	x->x_settime = clock_getsystime();
    }
}

static void upmet_float(t_upmet *x, t_float f)
{
    if (f != 0) {
    	
    	x->x_onoff = 1;
    	upmet_tick(x);
    } else {
    	clock_unset(x->x_clock);
    	x->x_onoff = 0;
    }
    x->x_hit = 1;
}

static void upmet_bang(t_upmet *x)
{
    upmet_float(x, 1);
}

static void upmet_stop(t_upmet *x)
{
    upmet_float(x, 0);
}

static void upmet_tempo(t_upmet *x, t_symbol *unitname, t_floatarg tempo)
{
    t_float unit;
    int samps;
    parsetimeunits(x, tempo, unitname, &unit, &samps);
    clock_setunit(x->x_clock, unit, samps);
    x->x_unit = unit;
    x->x_samps = samps;
}

static void upmet_free(t_upmet *x)
{
    clock_free(x->x_clock);
}

static void *upmet_new(t_symbol *unitname, t_floatarg f, t_floatarg tempo)
{
    t_upmet *x = (t_upmet *)pd_new(upmet_class);
    x->x_onoff = 0;
    x->x_unit = 1;
    x->x_samps = 0;
    upmet_ft1(x, f);
    x->x_hit = 0;
    x->x_clock = clock_new(x, (t_method)upmet_tick);
    outlet_new(&x->x_obj, gensym("bang"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    if (tempo != 0)
        upmet_tempo(x, unitname, tempo);
    return (x);
}

void upmet_setup(void)
{
    upmet_class = class_new(gensym("upmet"), (t_newmethod)upmet_new,
        (t_method)upmet_free, sizeof(t_upmet), 0,
            A_DEFFLOAT, A_DEFFLOAT, A_DEFSYM, 0);
    class_addbang(upmet_class, upmet_bang);
    class_addmethod(upmet_class, (t_method)upmet_stop, gensym("stop"), 0);
    class_addmethod(upmet_class, (t_method)upmet_ft1, gensym("ft1"),
        A_FLOAT, 0);
    class_addmethod(upmet_class, (t_method)upmet_tempo,
        gensym("tempo"), A_FLOAT, A_SYMBOL, 0);
    class_addfloat(upmet_class, (t_method)upmet_float);
}