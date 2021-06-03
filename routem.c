#include "m_pd.h"
/* modified from pd source */
/* -------------------------- routem ------------------------------ */

static t_class *routem_class;

typedef struct _routem
{
    t_object x_obj;
    t_shadylib_alist x_alist;
    t_outlet *x_acceptout;
    t_outlet *x_rejectout;
} t_routem;

static void routem_anything(t_routem *x, t_symbol *sel, int argc, t_atom *argv)
{
    t_routemelement *e;
    int nelement;
    if (x->x_type == A_SYMBOL)
    {
        for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            if (e->e_w.w_symbol == sel)
        {
            if (argc > 0 && argv[0].a_type == A_SYMBOL)
                outlet_anything(e->e_outlet, argv[0].a_w.w_symbol,
                    argc-1, argv+1);
            else outlet_list(e->e_outlet, 0, argc, argv);
            return;
        }
    }
    outlet_anything(x->x_rejectout, sel, argc, argv);
}

static void routem_list(t_routem *x, t_symbol *sel, int argc, t_atom *argv)
{
    t_routemelement *e;
    int nelement;
    if (x->x_type == A_FLOAT)
    {
        t_float f;
        if (!argc) return;
        if (argv->a_type != A_FLOAT)
            goto rejected;
        f = atom_getfloat(argv);
        for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            if (e->e_w.w_float == f)
        {
            if (argc > 1 && argv[1].a_type == A_SYMBOL)
                outlet_anything(e->e_outlet, argv[1].a_w.w_symbol,
                    argc-2, argv+2);
            else outlet_list(e->e_outlet, 0, argc-1, argv+1);
            return;
        }
    }
    else    /* symbol arguments */
    {
        if (argc > 1)       /* 2 or more args: treat as "list" */
        {
            for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            {
                if (e->e_w.w_symbol == &s_list)
                {
                    if (argc > 0 && argv[0].a_type == A_SYMBOL)
                        outlet_anything(e->e_outlet, argv[0].a_w.w_symbol,
                            argc-1, argv+1);
                    else outlet_list(e->e_outlet, 0, argc, argv);
                    return;
                }
            }
        }
        else if (argc == 0)         /* no args: treat as "bang" */
        {
            for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            {
                if (e->e_w.w_symbol == &s_bang)
                {
                    outlet_bang(e->e_outlet);
                    return;
                }
            }
        }
        else if (argv[0].a_type == A_FLOAT)    /* one float arg */
        {
            for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            {
                if (e->e_w.w_symbol == &s_float)
                {
                    outlet_float(e->e_outlet, argv[0].a_w.w_float);
                    return;
                }
            }
        }
        else if (argv[0].a_type == A_POINTER)    /* one pointer arg */
        {
            for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            {
                if (e->e_w.w_symbol == &s_pointer)
                {
                    outlet_pointer(e->e_outlet, argv[0].a_w.w_gpointer);
                    return;
                }
            }
        }
        else                                     /* one symbol arg */
        {
            for (nelement = x->x_nelement, e = x->x_vec; nelement--; e++)
            {
                if (e->e_w.w_symbol == &s_symbol)
                {
                    outlet_symbol(e->e_outlet, argv[0].a_w.w_symbol);
                    return;
                }
            }
        }
    }
 rejected:
    outlet_list(x->x_rejectout, 0, argc, argv);
}


static void routem_free(t_routem *x)
{
    shadylib_alist_clear(&x->x_alist);
}

static void *routem_new(t_symbol *s, int argc, t_atom *argv)
{
    t_routem *x = (t_routem *)pd_new(list_append_class);
    shadylib_alist_init(&x->x_alist);
    shadylib_alist_list(&x->x_alist, 0, argc, argv);
    x->x_acceptout = outlet_new(&x->x_obj, &s_list);
    x->x_rejectout = outlet_new(&x->x_obj, &s_list);
    inlet_new(&x->x_obj, &x->x_alist.l_pd, 0, 0);
    return (x);
}

void routem_setup(void)
{
    routem_class = class_new(gensym("routem"), (t_newmethod)routem_new,
        (t_method)routem_free, sizeof(t_routem), 0, A_GIMME, 0);
    class_addlist(routem_class, routem_list);
    class_addanything(routem_class, routem_anything);
}