#include "m_pd.h"

EXTERN void parsetimeunits(void *x, t_float amount, t_symbol *unitname,
    t_float *unit, int *samps);

static t_class *updel_class;

typedef struct _updel
{
    t_object x_obj;
    t_clock *x_clock;
    double x_settime;
/* guess we have to store the units too? */
    t_float x_unit;
    int x_samps;
    
    double x_deltime;
    double x_ttime;
    int x_onoff;

} t_updel;

static void updel_ft1(t_updel *x, t_floatarg g)
{
    if (g < 0) g = 0;
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

static void updel_tick(t_updel *x)
{
	x->x_onoff = 0;
    outlet_bang(x->x_obj.ob_outlet);
}

static void updel_bang(t_updel *x)
{
	x->x_onoff = 1;
	x->x_ttime = x->x_deltime;
	x->x_settime = clock_getsystime();
    clock_delay(x->x_clock, x->x_deltime);
}

static void updel_stop(t_updel *x)
{
    clock_unset(x->x_clock);
	x->x_onoff = 0;
}

static void updel_float(t_updel *x, t_float f)
{
	x->x_onoff = 0;
    updel_ft1(x, f);
    updel_bang(x);
}

static void updel_tempo(t_updel *x, t_symbol *unitname, t_floatarg tempo)
{
    t_float unit;
    int samps;
    parsetimeunits(x, tempo, unitname, &unit, &samps);
    clock_setunit(x->x_clock, unit, samps);
    x->x_unit = unit;
    x->x_samps = samps;
}

static void updel_free(t_updel *x)
{
    clock_free(x->x_clock);
}

static void *updel_new(t_symbol *unitname, t_floatarg f, t_floatarg tempo)
{
    t_updel *x = (t_updel *)pd_new(updel_class);
    x->x_onoff = 0;
    x->x_unit = 1;
    x->x_samps = 0;
    updel_ft1(x, f);
    x->x_clock = clock_new(x, (t_method)updel_tick);
    outlet_new(&x->x_obj, gensym("bang"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    if (tempo != 0)
        updel_tempo(x, unitname, tempo);
    return (x);
}

void updel_setup(void)
{
    updel_class = class_new(gensym("updel"), (t_newmethod)updel_new,
        (t_method)updel_free, sizeof(t_updel), 0,
            A_DEFFLOAT, A_DEFFLOAT, A_DEFSYM, 0);
    class_addbang(updel_class, updel_bang);
    class_addmethod(updel_class, (t_method)updel_stop, gensym("stop"), 0);
    class_addmethod(updel_class, (t_method)updel_ft1,
        gensym("ft1"), A_FLOAT, 0);
    class_addmethod(updel_class, (t_method)updel_tempo,
        gensym("tempo"), A_FLOAT, A_SYMBOL, 0);
    class_addfloat(updel_class, (t_method)updel_float);
}
