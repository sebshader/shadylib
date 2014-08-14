#include "m_pd.h"
#include <math.h>

#define UNITBIT32 1572864.  /* 3*2^19; bit 32 has place value 1 */

#ifdef IRIX
#include <sys/endian.h>
#endif

#if defined(__FreeBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
#include <machine/endian.h>
#endif

#if defined(__linux__) || defined(__CYGWIN__) || defined(__GNU__) || defined(ANDROID)
#include <endian.h>
#endif

#ifdef __MINGW32__
#include <sys/param.h>
#endif

#ifdef _MSC_VER
/* _MSVC lacks BYTE_ORDER and LITTLE_ENDIAN */
#define LITTLE_ENDIAN 0x0001
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN)                         
#error No byte order defined                                                    
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
# define HIOFFSET 1                                                              
# define LOWOFFSET 0                                                             
#else                                                                           
# define HIOFFSET 0    /* word offset to find MSB */                             
# define LOWOFFSET 1    /* word offset to find LSB */                            
#endif

#ifdef _MSC_VER
 typedef __int32 int32_t; /* use MSVC's internal type */
#elif defined(IRIX)
 typedef long int32_t;  /* a data type that has 32 bits */
#else
# include <stdint.h>  /* this is where int32_t is defined in C99 */
#endif

#define NORMHIPART 1094189056 //will this work on big-endian?

union tabfudge
{
    double tf_d;
    int32_t tf_i[2];
};

typedef struct _moop {
	t_object x_obj;
	t_outlet *x_out2;
    t_float x_f;
    t_float x_range;
    double x_phase; //keep track of time passed
    double x_period; //inverse of samples to play
    double x_sample; //actual sample value to read in buffer
    t_int x_num; //current number of repetitions
    t_float x_trns; //held transposition
    int x_hold; //sample & hold transposition or not?
} t_moop;

static t_class *moop_class;

static void moop_time(t_moop *x, t_symbol *s, int argc, t_atom *argv) {
	t_float time;
	if(argc > 0)
		if(argv[0].a_type == A_FLOAT) {
			time = atom_getfloat(argv);
			if(time == 0.0) {
				x->x_num = 0;
				return;
			} else {
				x->x_phase = 0.0;
				x->x_period = 1000/(time*sys_getsr());
				if(time < 0.0) x->x_sample = x->x_range;
				else x->x_sample = 0.0;
				if(argc == 1)
					if(time < 0.0) x->x_num = 2;
					else x->x_num = 1;
			}
		} else {
			post("moop~: list must be floats");
			return;
		}
	if(argc > 1) 
		if(argv[1].a_type == A_FLOAT) {
			t_int num = atom_getfloat(argv + 1);
			if(num <= 0) {
				x->x_num = 0;
				x->x_phase = 0.0;
				x->x_sample = 0.0;
			} else {
				if(time < 0.0) num++;
				x->x_num = num;
			}
		} else post("moop~: list must be floats");
	else if(argc == 1)
		if(time < 0.0) x->x_num = 2;
		else x->x_num = 1;
}

static void moop_range(t_moop *x, t_floatarg range) {
	if(range < 0) range = 0;
	x->x_range = range;
}

static void moop_hold(t_moop *x, t_floatarg hold) {
	x->x_hold = (hold != 0.0);
}

static t_int *moop_perform(t_int *w) {
	t_moop *x = (t_moop *)(w[1]);
	t_float *in = (t_float *)(w[2]);
	t_float *out1 = (t_float *)(w[3]);
	t_float *out2 = (t_float *)(w[4]);
	int n = (int)(w[5]);
	union tabfudge tf;
	t_float range = x->x_range;
	t_float sample = x->x_sample;
	t_int num = x->x_num;
	t_float period = x->x_period;
	tf.tf_d = x->x_phase;
	t_float trns;
    t_float tin;
	if(!num) goto done;
	tf.tf_d += (double)UNITBIT32;
	if(x->x_hold) {
		trns = x->x_trns;
		if(sample == 0.0) trns = copysign(exp2(*in/12), period);
		while (n) {
			n--;
			tin = *in++;
			*out1++ = sample;
    		*out2++ = tf.tf_d - UNITBIT32;
    		tf.tf_d += period;
    		sample += trns;
    		if (tf.tf_i[HIOFFSET] != NORMHIPART) {
    			num--;
				if(!num) {
					tf.tf_d = 0.0;
					goto done;
				}
				tf.tf_i[HIOFFSET] = NORMHIPART;
				if(period > 0) {
					trns = exp2(tin/12);
					sample = 0;
				} else {
					trns = - exp2(tin/12);
					sample = range;
				}
			}
		}
		x->x_trns = trns;
	} else {
		tin = *in;
		trns = copysign(exp2(tin/12), period);
		while (n) {
			n--;
			if(*in != tin) {
				trns = copysign(exp2(*in/12), period);
				tin = *in;
			}
			in++;
			*out1++ = sample;
    		*out2++ = tf.tf_d - UNITBIT32;
    		tf.tf_d += period;		
    		sample += trns;
    		if (tf.tf_i[HIOFFSET] != NORMHIPART) {
    			num--;
    			if(!num) {
					tf.tf_d = 0.0;
					goto done;
				}
    			tf.tf_i[HIOFFSET] = NORMHIPART;
				if(period > 0) sample = 0;
				else sample = range;
			}
		}
	}
	tf.tf_d -= UNITBIT32;
	x->x_sample = sample;
	done:
	x->x_num = num;
	while (n--) {
		*out1++ = sample;
		*out2++ = tf.tf_d;
	}
	x->x_phase = tf.tf_d;
    return (w+6);
}

static void moop_dsp(t_moop *x, t_signal **sp) {
	dsp_add(moop_perform, 5, x, sp[0]->s_vec, sp[1]->s_vec, 
		sp[2]->s_vec, sp[0]->s_n);
}

static void *moop_new(t_floatarg range, t_floatarg hold) {
	t_moop *x = (t_moop *)pd_new(moop_class);
	outlet_new(&x->x_obj, gensym("signal"));
    outlet_new(&x->x_obj, gensym("signal"));
    moop_range(x, range);
	moop_hold(x, hold);
	x->x_phase = 0.0;
	x->x_period = 0.0;
	x->x_sample = 0.0;
	x->x_num = 0;
	return (x);
}

void moop_tilde_setup(void) {
	moop_class = class_new(gensym("moop~"), (t_newmethod)moop_new, 0,
		sizeof(t_moop), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addmethod(moop_class, (t_method)moop_dsp, gensym("dsp"), A_CANT, 0);
	CLASS_MAINSIGNALIN(moop_class, t_moop, x_f);
	class_addmethod(moop_class, (t_method)moop_hold, gensym("hold"),
		A_DEFFLOAT, 0);
	class_addmethod(moop_class, (t_method)moop_time, gensym("times"),
		A_GIMME, 0);
	class_addmethod(moop_class, (t_method)moop_range, gensym("range"),
		A_DEFFLOAT, 0);
}
