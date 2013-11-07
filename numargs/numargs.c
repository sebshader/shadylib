
#include "m_pd.h"

static t_class *numargs_class;

typedef struct _numargs {
	t_object thisun;
	t_float numbr;
} t_numargs;

void *numargs_new(void){
	t_numargs *thisclass = (t_numargs *)pd_new(numargs_class);
	
	outlet_new(&thisclass->thisun, &s_float);
	
	int num;
	t_atom *dum;
	canvas_getargs(&num, &dum);
	thisclass->numbr = (t_float)(num);
	
	return (void *)thisclass;
}

void numargs_bang(t_numargs *thisclass){
	outlet_float(thisclass->thisun.ob_outlet, thisclass->numbr);
}

void numargs_setup(void){
	numargs_class = class_new(gensym("numargs"), (t_newmethod)numargs_new,
		0, sizeof(t_numargs), 0, 0);
	
	class_addbang(numargs_class, numargs_bang);
}
