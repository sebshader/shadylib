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
#include "shadylib.h"

typedef struct neadsrctl {
	shadylib_t_stage c_attack;
	shadylib_t_stage c_decay;
	shadylib_t_stage c_release;
	t_sample c_sustain;
	t_sample c_state;
	t_sample c_linr; //holds the state to release from
	int c_target;
} t_neadsrctl;

typedef struct neadsr {
	t_object x_obj;
	t_float x_sr;
	t_neadsrctl x_ctl;
} t_neadsr;

static void neadsr_float(t_neadsr *x, t_floatarg f) {
	if(f == 0.0) {
		if(x->x_ctl.c_target != -1) {
			x->x_ctl.c_target = -1;
			x->x_ctl.c_linr = x->x_ctl.c_state*x->x_ctl.c_release.base;
    	}
    } else {
    	x->x_ctl.c_target = 2;
    	if(f < 0.0) x->x_ctl.c_state = 0.0;
    }
}

static void neadsr_attack(t_neadsr *x, t_symbol *s, int argc, t_atom *argv) {
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

static void neadsr_decay(t_neadsr *x, t_symbol *s, int argc, t_atom *argv) {
	t_int samps;
	int abool;
	if(argc > 0) {
		samps = shadylib_ms2samps(atom_getfloat(argv), x->x_sr);
		abool = samps == x->x_ctl.c_decay.nsamp;
		if(argc > 1) {
			x->x_ctl.c_decay.nsamp = samps;
			shadylib_f2dxfade(atom_getfloat(argv + 1), &(x->x_ctl.c_decay), 
				abool);
		} else if(!abool) {
			x->x_ctl.c_decay.nsamp = samps;
			shadylib_ms2dxfade(&(x->x_ctl.c_decay));
		}
	}
}

static void neadsr_sustain(t_neadsr *x, t_floatarg f)
{
	x->x_ctl.c_sustain = fmax(0.0, fmin(1.0,f));
}

static void neadsr_release(t_neadsr *x, t_symbol *s, int argc, t_atom *argv) {
	t_int samps;
	int abool;
	if(argc > 0) {
		samps = shadylib_ms2samps(atom_getfloat(argv), x->x_sr);
		abool = samps == x->x_ctl.c_release.nsamp;
		if(argc > 1) {
			x->x_ctl.c_release.nsamp = samps;
			shadylib_f2rxfade(atom_getfloat(argv + 1), &(x->x_ctl.c_release), 
				abool);
		} else if(!abool) {
			x->x_ctl.c_release.nsamp = samps;
			shadylib_ms2rxfade(&(x->x_ctl.c_release));
		}
	}
}

static void neadsr_any(t_neadsr *x, t_symbol *s, int argc, t_atom *argv) {
	if(!strcmp(s->s_name, "curves")) { 
		switch(argc) {
			default:;
			case 3: if(argv[2].a_type == A_FLOAT) {
				shadylib_f2rxfade(atom_getfloat(argv + 2), 
					&(x->x_ctl.c_release), 1);
			}
			case 2: if(argv[1].a_type == A_FLOAT) {
				shadylib_f2dxfade(atom_getfloat(argv + 1), 
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
			case 4: 
				if(argv[3].a_type == A_FLOAT) {
					samps = shadylib_ms2samps(atom_getfloat(argv + 3), x->x_sr);
					if(samps != x->x_ctl.c_release.nsamp){
						x->x_ctl.c_release.nsamp = samps;
						shadylib_ms2rxfade(&(x->x_ctl.c_release));
					}
				}
			case 3: 
				if(argv[2].a_type == A_FLOAT) neadsr_sustain(x, 
				atom_getfloat(argv + 2));
			case 2: 
				if(argv[1].a_type == A_FLOAT) {
					samps = shadylib_ms2samps(atom_getfloat(argv + 1), x->x_sr);
					if(samps != x->x_ctl.c_decay.nsamp){
						x->x_ctl.c_decay.nsamp = samps;
						shadylib_ms2dxfade(&(x->x_ctl.c_decay));
					}
				}
			case 1: 
				if(argv[0].a_type == A_FLOAT) {
					samps = shadylib_ms2samps(atom_getfloat(argv), x->x_sr);
					if(samps != x->x_ctl.c_attack.nsamp){
						x->x_ctl.c_attack.nsamp = samps;
						shadylib_ms2axfade(&(x->x_ctl.c_attack));
					}
				}
			case 0:;
		}
	}
}

t_int *neadsr_perform(t_int *w)
{
    t_sample *out = (t_float *)(w[3]);
    t_neadsrctl *ctl = (t_neadsrctl *)(w[1]);
    t_sample state = ctl->c_state;
    t_sample sustain = ctl->c_sustain;
    int target = ctl->c_target;
    int n = (int)(w[2]);
    shadylib_t_stage stage;
	if (target == -1) {
		/*release*/
		if(state == 0.0) while(n--) *out++ = 0.0;
		else {
			stage = ctl->c_release;
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
					goto done;
				}
			}
		}
		goto done;
	} else if (target == 2){
		/* attack */
		stage = ctl->c_attack;
		while(n){
			n--;/*put outside of while so n != -1*/
			*out++ = state;
			#ifdef FP_FAST_FMA
			state = fma(state, stage.op, stage.base);
			#else
			state = state*stage.op + stage.base;
			#endif
			if(state >= 1.0) {
				state = 1.0;
				target = sustain;
				break;
			}
		}
	}
	if (n) {
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
				#ifdef FP_FAST_FMA
				state = fma(state, stage.op, stage.base);
				#else
				state = state*stage.op + stage.base;
				#endif
				if(state < sustain) state = sustain;
			} while(n && state > sustain);
		} else if (state < sustain) {
			stage = ctl->c_decay;
			stage.base *= (sustain + stage.lin);
			do {
				n--;
				*out++ = state;
				#ifdef FP_FAST_FMA
				state = fma(state, stage.op, stage.base);
				#else
				state = state*stage.op + stage.base;
				#endif
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
	if(sp[0]->s_sr != x->x_sr) {/*need to recalculate everything*/
		shadylib_t_stage thistage;
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
		thistage.base = (1 - thistage.op)/(1 - thistage.lin);
		x->x_ctl.c_decay = thistage;
		thistage = x->x_ctl.c_release;
		thistage.nsamp *= factor;
		thistage.op = pow(thistage.op, 1.0/factor);
		thistage.base = thistage.lin*(1 - thistage.op)/(1 - thistage.lin);
		x->x_ctl.c_release = thistage;
	}
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
    x->x_ctl.c_target = -1;
    x->x_sr = sys_getsr();
    x->x_ctl.c_attack.nsamp = ms2samps(attack, x->x_sr);
    shadylib_f2axfade(1-(log(1.0/3.0)/log(ENVELOPE_RANGE)),
    	&(x->x_ctl.c_attack), 0); /* 1/3 by default */
    x->x_ctl.c_decay.nsamp = ms2samps(decay, x->x_sr);
    shadylib_f2dxfade(0.0, &(x->x_ctl.c_decay), 0);
    neadsr_sustain(x, sustain);
    x->x_ctl.c_release.nsamp = ms2samps(release, x->x_sr);
    shadylib_f2rxfade(0.0, &(x->x_ctl.c_release), 0);
	return (void *)x;
}

void neadsr_tilde_setup(void)
{
    neadsr_class = class_new(gensym("neadsr~"), (t_newmethod)neadsr_new,
    	0, sizeof(t_neadsr), 0,  A_DEFFLOAT, 
			    A_DEFFLOAT, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(neadsr_class, (t_method)neadsr_float,
		    gensym("float"), A_FLOAT, 0);
    class_addmethod(neadsr_class, (t_method)neadsr_dsp, gensym("dsp"), A_CANT, 
            0); 
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