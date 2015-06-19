#include "m_pd.h"
#include "g_canvas.h"

static t_class *rooti_class;

typedef struct _rooti {
	t_object x_obj;
	t_canvas *canvas;
	t_outlet *s_out1, *s_out2, *f_out3;
	int level;
} t_rooti;

static void rootibang(t_rooti *x) {
	outlet_float(x->f_out3, x->level);
	outlet_symbol(x->s_out2, x->canvas->gl_name);
	outlet_symbol(x->s_out1, canvas_getdir(x->canvas));
}

static void *rooti_new() {
	t_canvas *current;
	int llevel = 0;
	t_rooti *x = (t_rooti *)pd_new(rooti_class);
	x->s_out1 = outlet_new(&x->x_obj, &s_symbol);
	x->s_out2 = outlet_new(&x->x_obj, &s_symbol);
	x->f_out3 = outlet_new(&x->x_obj, &s_float);
	current = canvas_getcurrent();
	while(current->gl_owner) {
		llevel++;
		current = current->gl_owner;
	}
	x->canvas = current;
	x->level = llevel;
	return(x);
}

void rootinfo_setup() {
	rooti_class = class_new(gensym("rootinfo"), (t_newmethod)rooti_new,
        0, sizeof(t_rooti), 0, 0);
    class_addbang(rooti_class, (t_method)rootibang);
}