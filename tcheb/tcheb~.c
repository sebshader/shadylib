#include "m_pd.h"
#include <math.h>

#define MAX_HARM 255.0
#define COSTAB_2 (COSTABSIZE >> 1)
static t_class* tcheb_tilde_class;

typedef struct _tcheb_tilde {
	t_object x_obj;
	t_float dumf;
	unsigned int ordr;
	unsigned int lngth;
	unsigned int instruct;
} t_tcheb_tilde;

t_int *tcheb_tilde_perform(t_int *w) {
	t_tcheb_tilde *x = (t_tcheb_tilde*) (w[1]);
	t_sample *in1 = (t_sample*) (w[2]), *in2 = (t_sample*) (w[3]),
		*out = (t_sample*) (w[4]);
	int n = (int) (w[5]);
	unsigned int o = x->ordr;
	unsigned int l = x->lngth;
	unsigned int dir = x->instruct;
	unsigned int newo;
	t_sample inord, t1, t2, tin;
	while(n--) {
		t1 = *in1; 
		if (t1 < -1.0) t1 = -1.0; else if (t1 > 1.0) t1 = 1.0;
		t2 = 2.0*t1*t1 - 1.0; inord = *in2++;
		if (inord < 1.0) inord = 1.0;
		else if (inord > MAX_HARM) inord = MAX_HARM;
		newo = ((unsigned int) (inord)) + 1;
		if(o != newo) {
			o = newo;
			
			//this is basically a binary stack for odd || even computations
			for(l = 0, dir = 0; newo > 2; l++, newo = (newo + 1) >> 1) {
				dir <<= 1;
				dir = (1 & newo) | dir;
			}
		}
		newo = dir;
		tin = t1;
		for(unsigned int i = 0; i < l; i++) {
			if(newo & 1) {
			/* 2t(n)*t(m) = t(n+m) + t(n-m) for m = n - 1 and n. odd and even
				idea is from fxt */
				t_sample temp = 2.0*t1;
				t1 = temp*t1 - 1.0;
				t2 = temp*t2 - tin;
			} else {
				t_sample temp = 2.0*t2;
				t1 = temp*t1 - tin;
				t2 = temp*t2 - 1.0;
			}
			newo >>= 1;
		}
		in1++;
		/* i'm not really prepared to use the pd cosine table
		but im gonna do it anyway */
		inord = (inord - floorf(inord)) * COSTAB_2;
		newo = (unsigned int)(inord);
		inord = inord - newo;
		newo += COSTAB_2;
		inord = (cos_table[newo + 1] - cos_table[newo])*inord + cos_table[newo];
		inord = inord * 0.5 + 0.5;
		*out++ = t1*(1.0 - inord) + t2*inord;
	}
	x->ordr = o;
	x->lngth = l;
	x->instruct = dir;
	return (w + 6);
}

void tcheb_tilde_dsp(t_tcheb_tilde *x, t_signal **sp) {
	dsp_add(tcheb_tilde_perform, 5, x, sp[0]->s_vec,  sp[1]->s_vec,
		sp[2]->s_vec, sp[0]->s_n);
}

void* tcheb_tilde_new(void) {
	t_tcheb_tilde* x = (t_tcheb_tilde*) pd_new(tcheb_tilde_class);
	
	x->ordr = 2;
	x->lngth = 0;
	
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	
	outlet_new(&x->x_obj, &s_signal);
	
	return (void*) x;
}
void tcheb_tilde_setup(void) {
	tcheb_tilde_class = class_new(gensym("tcheb~"), 
		(t_newmethod)tcheb_tilde_new, 0, sizeof(t_tcheb_tilde), CLASS_DEFAULT, 
		0);
	
	class_addmethod(tcheb_tilde_class, (t_method)tcheb_tilde_dsp, gensym("dsp"),
		0);
	CLASS_MAINSIGNALIN(tcheb_tilde_class, t_tcheb_tilde, dumf);
}

/*
void ntobstring (char *ar, unsigned int num, unsigned int length);

int main (int argc, char* argv[]) {
	char binstring[10];
	
	if(argc < 2){
		printf("must enter a number in order to get binary instructions\n");
		exit(0);
	}
	unsigned int num = atoi(argv[1]), instr, lngth;
	if (num > MAX_HARM) {
		printf("number over %i, setting to %i\n", MAX_HARM, MAX_HARM);
		num = MAX_HARM;
	}
	for(lngth = 0, instr = 0; num > 1; lngth++, num = (num + 1) >> 1) {
		instr <<= 1; // an integer stack, seems I can start at 2 though
		instr = (1 & num) | instr;
		printf("instr is %i\n", instr);
	}
	//only update num if it changes by an integer
	ntobstring(binstring, instr, lngth);
	//for (int i = 0; i < 12; i++) printf("index %i is %i\n", i, binstring[i]);
	printf("%.*s\n", lngth, binstring);
	return(0);
}



void ntobstring (char *ar, unsigned int num, unsigned int length) {
	for(int i = 1; i <= length; i++) {
		if((1 << (length-i)) & num)
			ar[i - 1] = 49;
		else ar[i - 1] = 48;
	}
}
*/