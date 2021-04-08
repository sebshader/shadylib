/*
 *   nead~.c  -  exponential attack decay envelope 
 *   Copyright (c) 2000-2014 by Tom Schouten & Seb Shader
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.*/

#include <string.h>
#include "shadylib.h"

typedef struct neadctl {
	t_shadylib_stage c_attack;
	t_shadylib_stage c_decay;
	t_sample c_linr; //holds the state to release from
	t_sample c_state;
	int c_target;
} t_neadctl;

typedef struct nead {
	t_object x_obj;
	t_float x_sr;
	t_neadctl x_ctl;
} t_nead;

static void nead_float(t_nead *x, t_floatarg f) {
	if(f == 0.0) {
    	if(x->x_ctl.c_target) {
    		x->x_ctl.c_target = 0;
    		x->x_ctl.c_linr = x->x_ctl.c_state*x->x_ctl.c_decay.base;
    	}
    } else {
    	x->x_ctl.c_target = 1;
    	if(f < 0.0) x->x_ctl.c_state = 0.0;
    }
}

static void nead_attack(t_nead* x, t_symbol* UNUSED(s), int argc,
    t_atom *argv) {
	t_int samps;
	int abool;
	if(argc > 0) {
		samps = shadylib_ms2samps(atom_getfloat(argv), x->x_sr);
		abool = samps == x->x_ctl.c_attack.nsamp;
		if(argc > 1) {
			x->x_ctl.c_attack.nsamp = samps;
			shadylib_f2axfade(atom_getfloat(argv + 1), &(x->x_ctl.c_attack), 
				abool);
		} else if(!abool) {
			x->x_ctl.c_attack.nsamp = samps;
			shadylib_ms2axfade(&(x->x_ctl.c_attack));
		}
	}
}

static void nead_decay(t_nead* x, t_symbol* UNUSED(s), int argc, t_atom *argv) {
	t_int samps;
	int abool;
	if(argc > 0) {
		samps = shadylib_ms2samps(atom_getfloat(argv), x->x_sr);
		abool = samps == x->x_ctl.c_decay.nsamp;
		if(argc > 1) {
			x->x_ctl.c_decay.nsamp = samps;
			shadylib_f2rxfade(atom_getfloat(argv + 1), &(x->x_ctl.c_decay), 
				abool);
		} else if(!abool) {
			x->x_ctl.c_decay.nsamp = samps;
			shadylib_ms2rxfade(&(x->x_ctl.c_decay));
		}
	}
}

static void nead_any(t_nead *x, t_symbol *s, int argc, t_atom *argv) {
	if(!strcmp(s->s_name, "curves")) { 
		switch(argc) {
			default:;
			case 2: if(argv[1].a_type == A_FLOAT) {
				shadylib_f2rxfade(atom_getfloat(argv + 1), 
					&(x->x_ctl.c_decay), 1);
			}
			case 1: if(argv[0].a_type == A_FLOAT) {
				shadylib_f2axfade(atom_getfloat(argv), &(x->x_ctl.c_attack), 1);
			}
			case 0:;
		}
	} else {
		t_int samps;
		switch(argc) {
			default:;
			case 2: 
				if(argv[1].a_type == A_FLOAT) {
					samps = shadylib_ms2samps(atom_getfloat(argv + 1), x->x_sr);
					if(samps != x->x_ctl.c_decay.nsamp){
						x->x_ctl.c_decay.nsamp = samps;
						shadylib_ms2rxfade(&(x->x_ctl.c_decay));
					}
				}
			case 1: 
				if(argv[0].a_type == A_FLOAT) {
					samps = shadylib_ms2samps(atom_getfloat(argv), x->x_sr);
					if(samps != x->x_ctl.c_attack.nsamp) {
						x->x_ctl.c_attack.nsamp = samps;
						shadylib_ms2axfade(&(x->x_ctl.c_attack));
					}
				}
			case 0:;
		}
	}
}

t_int *nead_perform(t_int *w)
{
    t_sample *out = (t_float *)(w[3]);
    t_neadctl *ctl = (t_neadctl *)(w[1]);
    t_sample state = ctl->c_state;
    int n = (int)(w[2]);
    int target = ctl->c_target;
    t_shadylib_stage stage;
	if (target){
		/* attack */
		stage = ctl->c_attack;
		while(n){
			n--;/*put inside of while so n != -1*/
			*out++ = state;
			#ifdef FP_FAST_FMA
			state = fma(state, stage.op, stage.base);
			#else
			state = state*stage.op + stage.base;
			#endif
			if(state >= 1.0) {
				state = 1.0;
				target = 0;
				ctl->c_linr = ctl->c_decay.base;
				break;
			}
		}
	}
	if (!target) {
		/*decay*/
		if(state == 0.0) while(n--) *out++ = 0.0;
		else {
			stage = ctl->c_decay;
			stage.base = ctl->c_linr;
			while(n--){
				*out++ = state;
				#ifdef FP_FAST_FMA
				state = fma(state, stage.op, stage.base);
				#else
				state = state*stage.op + stage.base;
				#endif
				if(state <= 0.0) {
					state = 0.0;
					for(;n;n--) *out++ = state;
				}
			}
		}
	}
    /* save state */

    ctl->c_state = SHADYLIB_IS_DENORMAL(state) ? 0 : state;
    ctl->c_target = target;
    return (w+4);
}

void nead_dsp(t_nead *x, t_signal **sp)
{
	if(sp[0]->s_sr != x->x_sr) {/*need to recalculate everything*/
		t_shadylib_stage thistage;
		float factor = sp[0]->s_sr/x->x_sr;
		x->x_sr = sp[0]->s_sr;
		thistage = x->x_ctl.c_attack;
		thistage.nsamp *= factor;
		/*should be better because they are low powers/roots? idk tho*/
		thistage.op = pow(thistage.op, 1.0/factor);
		thistage.base = (1 - thistage.op)/(1 - thistage.lin);
		x->x_ctl.c_attack = thistage;
		thistage = x->x_ctl.c_decay;
		thistage.nsamp *= factor;
		thistage.op = pow(thistage.op, 1.0/factor);
		thistage.base = thistage.lin*(1 - thistage.op)/(1 - thistage.lin);
		x->x_ctl.c_decay = thistage;
	}
    dsp_add(nead_perform, 3, &x->x_ctl, sp[0]->s_n, sp[0]->s_vec);
}                                  

t_class *nead_class;

void *nead_new(t_floatarg attack, t_floatarg decay) {
    t_nead *x = (t_nead *)pd_new(nead_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("attack"));  
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("decay"));  
    outlet_new(&x->x_obj, &s_signal);
    x->x_ctl.c_state = 0;
    x->x_ctl.c_target = 0;
    x->x_sr = sys_getsr();
    x->x_ctl.c_attack.nsamp = shadylib_ms2samps(attack, x->x_sr);
    shadylib_f2axfade(1-(log(1.0/3.0)/log(SHADYLIB_ENVELOPE_RANGE)), 
    	&(x->x_ctl.c_attack), 0); /* 1/3 by default */
    x->x_ctl.c_decay.nsamp = shadylib_ms2samps(decay, x->x_sr);
    shadylib_f2rxfade(0.0, &(x->x_ctl.c_decay), 0);
	return (void *)x;
}

void nead_tilde_setup(void)
{
    nead_class = class_new(gensym("nead~"), (t_newmethod)nead_new,
    	0, sizeof(t_nead), 0,  A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(nead_class, (t_method)nead_float,
		    &s_float, A_FLOAT, 0);
    class_addmethod(nead_class, (t_method)nead_dsp, gensym("dsp"), A_CANT, 0); 
    class_addmethod(nead_class, (t_method)nead_attack,
		    gensym("attack"), A_GIMME, 0);
    class_addmethod(nead_class, (t_method)nead_decay,
		    gensym("decay"), A_GIMME, 0);
	class_addanything(nead_class, (t_method)nead_any);
}