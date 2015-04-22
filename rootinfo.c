#include "m_pd.h"
#include "g_canvas.h"

static t_class *rooti_class;

typedef struct _rooti {
	t_object x_obj;
	t_symbol *s_path;
	int level;
	t_outlet *s_out1, *f_out2;
} t_rooti;

static void rootibang(t_rooti *x) {
	outlet_float(x->f_out2, x->level);
	outlet_symbol(x->s_out1, x->s_path);
}

static void *rooti_new() {
	t_canvas *current;
	int llevel = 0;
	t_rooti *x = (t_rooti *)pd_new(rooti_class);
	x->s_out1 = outlet_new(&x->x_obj, &s_symbol);
	x->f_out2 = outlet_new(&x->x_obj, &s_float);
	current = canvas_getcurrent();
	while(current->gl_owner) {
		llevel++;
		current = current->gl_owner;
	}
	x->s_path = canvas_getdir(current);
	x->level = llevel;
	return(x);
}

void rootinfo_setup() {
	rooti_class = class_new(gensym("rootinfo"), (t_newmethod)rooti_new,
        0, sizeof(t_rooti), 0, 0);
    class_addbang(rooti_class, (t_method)rootibang);
}