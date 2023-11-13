#include "shadylib.h"

static t_class *list_relim_class;

typedef struct _list_relim
{
    t_object x_obj;
    t_symbol *x_s;
} t_list_relim;

static void *list_relim_new(t_symbol *s)
{
    t_list_relim *x = (t_list_relim *)pd_new(list_relim_class);
    outlet_new(&x->x_obj, &s_list);
    symbolinlet_new(&x->x_obj, &x->x_s);
    x->x_s = s;
    return (x);
}

static void list_relim_list(t_list_relim *x, t_symbol* s,
    int argc, t_atom *argv)
{
    t_symbol *c = x->x_s;
    size_t strlength = strlen(c->s_name);
    size_t strdiff = (MAXPDSTRING - 1) - strlength;
    char namebuf[MAXPDSTRING];
    strcpy(namebuf, c->s_name);
    t_atom *outv;
    SHADYLIB_ATOMS_ALLOCA(outv, argc);
    shadylib_atoms_copy(argc, argv, outv);
    for(int i = 0; i < argc; i++){
        if(outv[i].a_type == A_SYMBOL) {
            const char *against = atom_getsymbol(outv+i)->s_name;
            size_t alength = strlen(against);
            for(size_t k = 0; k < alength; k += strlength)
                if (strncmp(c->s_name, against + k, strlength) != 0)
                    goto cont;
            strncpy(namebuf+strlength, against, strdiff);
            outv[i].a_w.w_symbol = gensym(namebuf);
        }
        cont:;
    }
    outlet_list(x->x_obj.ob_outlet, s, argc, outv);
    SHADYLIB_ATOMS_FREEA(outv, argc);
}

static void list_relim_anything(t_list_relim *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_symbol *c = x->x_s, *outsym = s;
    size_t strlength = strlen(c->s_name), alength;
    size_t strdiff = (MAXPDSTRING - 1) - strlength;
    const char *against;
    char namebuf[MAXPDSTRING];
    strcpy(namebuf, c->s_name);
    t_atom *outv;
    SHADYLIB_ATOMS_ALLOCA(outv, argc);
    shadylib_atoms_copy(argc, argv, outv);
    for(int i = 0; i < argc; i++){
        if(outv[i].a_type == A_SYMBOL) {
            against = atom_getsymbol(outv+i)->s_name;
            alength = strlen(against);
            for(size_t k = 0; k < alength; k += strlength)
                if (strncmp(c->s_name, against + k, strlength) != 0)
                    goto cont;
            strncpy(namebuf+strlength, against, strdiff);
            outv[i].a_w.w_symbol = gensym(namebuf);
        }
        cont:;
    }
    against = s->s_name;
    alength = strlen(against);
    for(size_t k = 0; k < alength; k += strlength)
        if (strncmp(c->s_name, against + k, strlength) != 0)
            goto send;
    strncpy(namebuf+strlength, against, strdiff);
    outsym = gensym(namebuf);
    send:
    outlet_anything(x->x_obj.ob_outlet, outsym, argc, outv);
    SHADYLIB_ATOMS_FREEA(outv, argc);
}

void setup_list0x2drelim(void)
{
    list_relim_class = class_new(gensym("list-relim"),
        (t_newmethod)list_relim_new, 0,
        sizeof(t_list_relim), 0, A_DEFSYM, 0);
    class_addlist(list_relim_class, list_relim_list);
    class_addanything(list_relim_class, list_relim_anything);
}
