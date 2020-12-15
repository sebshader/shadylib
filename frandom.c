#include "m_pd.h"

//linear congruential method from "Algorithms in C" by Robert Sedgewick
#define m 16777216 // 2^24: 24 digits are 0
#define m1 4096 // 2^12
#define b 2375621 /* arbitrary number ending in ...(even digit)21
                       with 1 digit less than m */

/*below is a program to test the number generator
//chisquare: test if within 128 (2*sqrt(4096)) of 4096

#include <stdio.h>

#define m 16777216 
#define m1 4096
#define b 2375621

//you can (and should) substitute 60000 with other values greater than 40960
#define N 60000

int mult(int p, int q) {
	int p1, p0, q1, q0;
	p1 = p/m1; p0 = p%m1;
	q1 = q/m1; q0 = q%m1;
	return (((p0*q1 + p1*q0)%m1)*m1 + p0*q0)%m;
}

int randomtest(int in) {
	in = (mult(in, b) + 1)%m;
	return in;
}

int tab[m1];

int main (int argc, char **argv) {
	int state = 0, i;
	float result;
	for(i = 0; i < m1; i++) tab[i] = 0;
	for(i = 0; i < N; i++) {
		state = randomtest(state); 
		tab[state/m1]++;
	}
	state = 0;
	for(i = 0; i < m1; i++) {
		state += tab[i]*tab[i];
	}
	printf("state = %i\n", state);
	result = (((float)state)/N)*m1 - N;
	printf("result = %f\n", result);
	printf("difference: %f (absolute value should be < 128)\n", m1 - result);
}

*/

static t_class *frandom_class;

typedef struct _frandom {
	t_object x_obj;
	t_float x_range;
	unsigned int x_state; //current state (int)
} t_frandom;

// 579 = b/m1, 4037 = b%m1
static int mult(unsigned int p) {
	unsigned int p1, p0;
	p1 = p/m1; p0 = p%m1;
	return (((p0*579 + p1*4037)%m1)*m1 + p0*4037)%m;
}

static inline int randlcm(unsigned int in) {
	in = (mult(in) + 1)%m;
	return in;
}

static void frandom_seed(t_frandom *x, t_floatarg seed) {
	unsigned int modman = seed;
	x->x_state = modman%m;
}

static void frandom_bang(t_frandom *x) {
    unsigned int state = x->x_state;

	state = randlcm(state);
	x->x_state = state;
	outlet_float(x->x_obj.ob_outlet, state*(x->x_range/16777215.0f));
}

// b = 4103221
static int frandom_makeseed(void) {
	unsigned int p1, p0;
	static unsigned int nextseed = 13458715;

	p1 = nextseed/m1; p0 = nextseed%m1;
	nextseed = (((p0*1001 + p1*3125)%m1)*m1 + p0*3125)%m;
	return nextseed;
}

static void *frandom_new(t_symbol *s, int argc, t_atom *argv) {
	unsigned int state; 
    t_frandom *x = (t_frandom *)pd_new(frandom_class);

	switch(argc) {
		default:
			state = atom_getint(argv + 1);
		case 1:
			x->x_range = atom_getfloat(argv);
		case 0:;
	}
	switch(argc) {
		case 0:
			x->x_range = 1.0;
		case 1:
			state = frandom_makeseed();
		default:;
	}
	floatinlet_new(&x->x_obj, &x->x_range);
    outlet_new(&x->x_obj, &s_float);
    state %= m;
    x->x_state = state;
    return (x);
}

void frandom_setup(void) {
    frandom_class = class_new(gensym("frandom"), (t_newmethod)frandom_new, 0,
        sizeof(t_frandom), CLASS_DEFAULT, A_GIMME, 0);
    class_addmethod(frandom_class, (t_method)frandom_seed, gensym("seed"), A_DEFFLOAT, 0);
	class_addbang(frandom_class, frandom_bang);
}
