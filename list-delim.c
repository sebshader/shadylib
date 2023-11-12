#include "shadylib.h"

static t_class *list_delim_class;

typedef struct _list_delim
{
    t_object x_obj;
    t_symbol *x_s;
} t_list_delim;

static void *list_delim_new(t_symbol *s)
{
    t_list_delim *x = (t_list_delim *)pd_new(list_delim_class);
    outlet_new(&x->x_obj, &s_list);
    symbolinlet_new(&x->x_obj, &x->x_s);
    x->x_s = s;
    return (x);
}

static inline int testatoms(t_symbol *tester, t_atom *against){
    if (against->a_type == A_SYMBOL)
        return tester == atom_getsymbol(against);
    else return 0;
}

static void list_delim_list(t_list_delim *x, t_symbol* s,
    int argc, t_atom *argv)
{
    t_symbol *c = x->x_s;
    size_t strlength = strlen(c->s_name);
    char namebuf[MAXPDSTRING + 1];
    t_atom *outv;
    int outc = 0, firstindex = 0, i, firstpass = 0;
    for(i = 0; i < argc; i++){
        if(testatoms(c, argv+i)) {
            if(firstpass) {
                SHADYLIB_ATOMS_ALLOCA(outv, outc);
                shadylib_atoms_copy(outc, argv+firstindex, outv);
                for(int j = 0; j < outc; j++) {
                    if(outv[j].a_type == A_SYMBOL) {
                        const char *against = atom_getsymbol(outv+j)->s_name;
                        size_t alength = strlen(against);
                        for(size_t k = 0; k < alength; k += strlength)
                            if (strncmp(c->s_name, against + k, strlength)
                                != 0)
                                goto cont;
                        strncpy(namebuf, against+strlength, MAXPDSTRING);
                        outv[j].a_w.w_symbol = gensym(namebuf);
                    }
                    cont:;
                }
                outlet_list(x->x_obj.ob_outlet, s, outc, outv);
                SHADYLIB_ATOMS_FREEA(outv, outc);
                firstpass = 0;
                outc = 0;
            }
        } else {
            if(!firstpass) {
                firstindex = i;
                firstpass = 1;
            }
            outc++;
        }
    }
    if(firstpass) {
        SHADYLIB_ATOMS_ALLOCA(outv, outc);
        shadylib_atoms_copy(outc, argv+firstindex, outv);
        for(int j = 0; j < outc; j++) {
            if(outv[j].a_type == A_SYMBOL) {
                const char *against = atom_getsymbol(outv+j)->s_name;
                size_t alength = strlen(against);
                for(size_t k = 0; k < alength; k += strlength)
                    if (strncmp(c->s_name, against + k, strlength) != 0)
                        goto cont2;
                strncpy(namebuf, against+strlength, MAXPDSTRING);
                outv[j].a_w.w_symbol = gensym(namebuf);
            }
            cont2:;
        }
        outlet_list(x->x_obj.ob_outlet, s, outc, outv);
        SHADYLIB_ATOMS_FREEA(outv, outc);
    }
}

static void list_delim_anything(t_list_delim *x, t_symbol *s,
    int argc, t_atom *argv)
{
    t_atom *outv;
    SHADYLIB_ATOMS_ALLOCA(outv, argc+1);
    SETSYMBOL(outv, s);
    shadylib_atoms_copy(argc, argv, outv + 1);
    list_delim_list(x, &s_list, argc+1, outv);
    SHADYLIB_ATOMS_FREEA(outv, argc+1);
}

void setup_list0x2ddelim(void)
{
    list_delim_class = class_new(gensym("list-delim"),
        (t_newmethod)list_delim_new, 0,
        sizeof(t_list_delim), 0, A_DEFSYM, 0);
    class_addlist(list_delim_class, list_delim_list);
    class_addanything(list_delim_class, list_delim_anything);
}
