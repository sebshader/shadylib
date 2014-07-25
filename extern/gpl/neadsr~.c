/*
 *   neadsr~.c  -  exponential attack decay sustain release envelope 
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
#include "nextlib_util.h"

typedef struct neadsrctl {
	t_stage c_attack;
	t_stage c_decay;
	t_stage c_release;
	t_float c_sustain;
	t_float c_state;
	t_float c_linr; //holds the state to release from
	t_float c_target;
} t_neadsrctl;

typedef struct neadsr {
	t_object x_obj;
	t_neadsrctl x_ctl;
	t_float x_sr;
} t_neadsr;

static void neadsr_float(t_neadsr *x, t_floatarg f) {
	if(f == 0.0) {
    	x->x_ctl.c_target = 0.0;
    	x->x_ctl.c_linr = x->x_ctl.c_state;
    } else {
    	x->x_ctl.c_target = 1.0;
    	if(f < 0.0) x->x_ctl.c_state = 0.0;
    }
}

static void neadsr_attack(t_neadsr *x, t_symbol *s, int argc, t_atom *argv) {
	switch(argc) {
		default:;
		case 2: 
			ms2samps(atom_getfloat(argv), &(x->x_ctl.c_attack));
			f2axfade(atom_getfloat(argv + 1), &(x->x_ctl.c_attack));
			break;
		case 1: ms2axfade(atom_getfloat(argv), &(x->x_ctl.c_attack));
		case 0:;
	}
}

static void neadsr_decay(t_neadsr *x, t_symbol *s, int argc, t_atom *argv) {
	switch(argc) {
		default:;
		case 2:
			ms2samps(atom_getfloat(argv), &(x->x_ctl.c_decay));
			f2dxfade(atom_getfloat(argv + 1), &(x->x_ctl.c_decay));
			break;
		case 1: ms2dxfade(atom_getfloat(argv), &(x->x_ctl.c_decay));
		case 0:;
	}
}

static void neadsr_sustain(t_neadsr *x, t_floatarg f)
{
	if(f>ENVELOPE_MAX) f = ENVELOPE_MAX;
	if(f<ENVELOPE_RANGE) f = ENVELOPE_RANGE;

	x->x_ctl.c_sustain = f;
}

static void neadsr_release(t_neadsr *x, t_symbol *s, int argc, t_atom *argv) {
	switch(argc) {
		default:;
		case 2:
			ms2samps(atom_getfloat(argv), &(x->x_ctl.c_release));
			f2rxfade(atom_getfloat(argv + 1), &(x->x_ctl.c_release));
			break;
		case 1: ms2rxfade(atom_getfloat(argv), &(x->x_ctl.c_release));
		case 0:;
	}
}

static void neadsr_any(t_neadsr *x, t_symbol *s, int argc, t_atom *argv) {
	if(!strcmp(s->s_name, "curves")) { 
		switch(argc) {
			default:;
			case 3: if(argv[2].a_type == A_FLOAT) {
				f2rxfade(atom_getfloat(argv + 2), &(x->x_ctl.c_release));
			}
			case 2: if(argv[1].a_type == A_FLOAT) {
				f2dxfade(atom_getfloat(argv + 1), &(x->x_ctl.c_decay));
			}
			case 1: if(argv[0].a_type == A_FLOAT) {
				f2axfade(atom_getfloat(argv), &(x->x_ctl.c_attack));
			}
			case 0:;
		}
	} else {
		switch(argc) {
			default:;
			case 4: 
				if(argv[3].a_type == A_FLOAT) ms2rxfade(atom_getfloat(argv + 3), &(x->x_ctl.c_release));
			case 3: 
				if(argv[2].a_type == A_FLOAT) neadsr_sustain(x, atom_getfloat(argv + 2));
			case 2: 
				if(argv[1].a_type == A_FLOAT) ms2dxfade(atom_getfloat(argv + 1), &(x->x_ctl.c_decay));
			case 1: 
				if(argv[0].a_type == A_FLOAT) ms2axfade(atom_getfloat(argv), &(x->x_ctl.c_attack));
			case 0:;
		}
	}
}

t_int *neadsr_perform(t_int *w)
{
    t_float *out = (t_float *)(w[3]);
    t_neadsrctl *ctl = (t_neadsrctl *)(w[1]);
    t_float state = ctl->c_state;
    t_float sustain = ctl->c_sustain;
    t_float target = ctl->c_target;
    t_int n = (t_int)(w[2]);
    t_stage stage;
	if (target == 0.0) {
		/*release*/
		if(state == 0.0) while(n--) *out++ = 0.0;
		else {
			stage = ctl->c_release;
			if(stage.lin == 1.0) stage.base *= ctl->c_linr;
			while(n--){
				*out++ = state;
				state = state*stage.op + stage.base;
				if(state <= 0.0) {
					state = 0.0;
					while(n) {
						n--;
						*out++ = state;
					}
				}
			}
		}
		goto done;
	} else if (target == 1.0f){
		/* attack */
		stage = ctl->c_attack;
		while(n){
			n--;/*put outside of while so n != -1*/
			*out++ = state;
			state = state*stage.op + stage.base;
			if(state >= 1.0) {
				state = 1.0;
				target = sustain;
				break;
			}
		}
	}
	if (target != 0.0f && n) {
		/*decay*/
		if(state == sustain) {
			while(n--) *out++ = state;
			goto done;
		} else if(state > sustain) {
			stage = ctl->c_decay;
			stage.base *= (sustain - stage.lin);
			do {
				n--;
				*out++ = state;
				state = state*stage.op + stage.base;
				if(state < sustain) state = sustain;
			} while(n && state > sustain);
		} else if (state < sustain) {
			stage = ctl->c_decay;
			stage.base *= (sustain + stage.lin);
			do {
				n--;
				*out++ = state;
				state = state*stage.op + stage.base;
				if(state > sustain) state = sustain;
			} while(n && state < sustain);
		}
		while(n--) *out++ = state;
	}
	done:
    /* save state */
    ctl->c_state = IS_DENORMAL(state) ? 0 : state;
    ctl->c_target = target;
    return (w+4);
}

void neadsr_dsp(t_neadsr *x, t_signal **sp)
{
    x->x_sr = sp[0]->s_sr;
    dsp_add(neadsr_perform, 3, &x->x_ctl, sp[0]->s_n, sp[0]->s_vec);
}                                  

t_class *neadsr_class;

void *neadsr_new(t_floatarg attack, t_floatarg decay, 
		t_floatarg sustain, t_floatarg release)
{
    t_neadsr *x = (t_neadsr *)pd_new(neadsr_class);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("attack"));  
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("decay"));  
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("sustain"));
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("release"));
    outlet_new(&x->x_obj, gensym("signal"));
    x->x_ctl.c_state = 0;
    x->x_ctl.c_target = 0;
    ms2samps(attack, &(x->x_ctl.c_attack));
    f2axfade(1-(log(1.0/3.0)/log(ENVELOPE_RANGE)), &(x->x_ctl.c_attack)); /* 1/3 by default */
    ms2samps(decay, &(x->x_ctl.c_decay));
    f2dxfade(0.0, &(x->x_ctl.c_decay));
    neadsr_sustain(x, sustain);
    ms2samps(release, &(x->x_ctl.c_release));
    f2rxfade(0.0, &(x->x_ctl.c_release));
	
	return (void *)x;
}

void neadsr_tilde_setup(void)
{
    neadsr_class = class_new(gensym("neadsr~"), (t_newmethod)neadsr_new,
    	0, sizeof(t_neadsr), 0,  A_DEFFLOAT, 
			    A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(neadsr_class, (t_method)neadsr_float,
		    gensym("float"), A_FLOAT, 0);
    class_addmethod(neadsr_class, (t_method)neadsr_dsp, gensym("dsp"), 0); 
    class_addmethod(neadsr_class, (t_method)neadsr_attack,
		    gensym("attack"), A_GIMME, 0);
    class_addmethod(neadsr_class, (t_method)neadsr_decay,
		    gensym("decay"), A_GIMME, 0);
    class_addmethod(neadsr_class, (t_method)neadsr_sustain,
		    gensym("sustain"), A_FLOAT, 0);
    class_addmethod(neadsr_class, (t_method)neadsr_release,
		    gensym("release"), A_GIMME, 0);
	class_addanything(neadsr_class, (t_method)neadsr_any);
}