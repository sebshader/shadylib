/* S.S. : this is taken from Maxlib. modded to "trim" certain messages

.. do I have to free proxies? */

/* --------------------------  prepender   ------------------------------------- */
/*                                                                              */
/* Opposit to croute.                                                           */
/* Written by Olaf Matthes <olaf.matthes@gmx.de>                                */
/* Get source at http://www.akustische-kunst.org/puredata/                      */
/*                                                                              */
/* This program is free software; you can redistribute it and/or                */
/* modify it under the terms of the GNU General Public License                  */
/* as published by the Free Software Foundation; either version 2               */
/* of the License, or (at your option) any later version.                       */
/*                                                                              */
/* This program is distributed in the hope that it will be useful,              */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of               */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                */
/* GNU General Public License for more details.                                 */
/*                                                                              */
/* You should have received a copy of the GNU General Public License            */
/* along with this program; if not, write to the Free Software                  */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.  */
/*                                                                              */
/* Based on PureData by Miller Puckette and others.                             */
/*                                                                              */
/* ---------------------------------------------------------------------------- */

#include "shadylib.h"

#define MAX_INLET 256

typedef struct proxy
{
    t_object obj;
    t_atom id;
    struct prepender *x;        /* we'll put the other struct in here */
} t_proxy;

typedef struct prepender
{
  t_object x_obj;
  t_outlet *x_outlet;
  t_int    x_ninstance;
  t_proxy **x_proxies;
  t_atom   x_id;
} t_prepender;

static void prepender_any(t_prepender *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom *outv;

    if(x->x_id.a_type == A_FLOAT) {
        SHADYLIB_ATOMS_ALLOCA(outv, argc + 2);
        shadylib_atoms_copy(argc, argv, outv + 2);
        argc += 2;
        SETSYMBOL(outv+1, s);
        *outv = x->x_id;
        outlet_list(x->x_obj.ob_outlet, &s_list, argc, outv);
    } else {
        SHADYLIB_ATOMS_ALLOCA(outv, argc + 1);
        shadylib_atoms_copy(argc, argv, outv + 1);
        argc++;
        SETSYMBOL(outv, s);
        outlet_anything(x->x_obj.ob_outlet, x->x_id.a_w.w_symbol, argc, outv);
    }
    SHADYLIB_ATOMS_FREEA(outv, argc);
}

static void prepender_list(t_prepender *x, t_symbol* SHADYLIB_UNUSED(s),
    int argc, t_atom *argv)
{
    t_atom *outv;

    if(x->x_id.a_type == A_FLOAT) {
        SHADYLIB_ATOMS_ALLOCA(outv, argc + 1);
        shadylib_atoms_copy(argc, argv, outv + 1);
        *outv = x->x_id;
        argc++;
        outlet_list(x->x_obj.ob_outlet, &s_list, argc, outv);
    } else {
        SHADYLIB_ATOMS_ALLOCA(outv, argc);
        shadylib_atoms_copy(argc, argv, outv);
        outlet_anything(x->x_obj.ob_outlet, x->x_id.a_w.w_symbol, argc, outv);
    }
    SHADYLIB_ATOMS_FREEA(outv, argc);
}


static void prepender_float(t_prepender *x, t_floatarg f)
{
    if(x->x_id.a_type == A_FLOAT) {
        t_atom *outv;
        SHADYLIB_ATOMS_ALLOCA(outv, 2);
        SETFLOAT(outv + 1, f);
        *outv = x->x_id;
        outlet_list(x->x_obj.ob_outlet, &s_list, 2, outv);
        #if 2 >= SHADYLIB_LIST_NGETBYTE
            SHADYLIB_ATOMS_FREEA(outv, 2);
        #endif
    } else {
        t_atom fman;
        SETFLOAT(&fman, f);
        outlet_anything(x->x_obj.ob_outlet, x->x_id.a_w.w_symbol, 1, &fman);
    }

}

static void proxy_any(t_proxy *x, t_symbol *s, int argc, t_atom *argv)
{
    t_atom *outv;

    if(x->id.a_type == A_FLOAT) {
        SHADYLIB_ATOMS_ALLOCA(outv, argc + 2);
        shadylib_atoms_copy(argc, argv, outv + 2);
        argc += 2;
        SETSYMBOL(outv+1, s);
        *outv = x->id;
        outlet_list(x->x->x_obj.ob_outlet, &s_list, argc, outv);
    } else {
        SHADYLIB_ATOMS_ALLOCA(outv, argc + 1);
        shadylib_atoms_copy(argc, argv, outv + 1);
        argc++;
        SETSYMBOL(outv, s);
        outlet_anything(x->x->x_obj.ob_outlet, x->id.a_w.w_symbol, argc, outv);
    }
    SHADYLIB_ATOMS_FREEA(outv, argc);
}

static void proxy_list(t_proxy *x, t_symbol* SHADYLIB_UNUSED(s),
    int argc, t_atom *argv)
{
    t_atom *outv;

    if(x->id.a_type == A_FLOAT) {
        SHADYLIB_ATOMS_ALLOCA(outv, argc + 1);
        shadylib_atoms_copy(argc, argv, outv + 1);
        argc++;
        *outv = x->id;
        outlet_list(x->x->x_obj.ob_outlet, &s_list, argc, outv);
    } else {
        SHADYLIB_ATOMS_ALLOCA(outv, argc);
        shadylib_atoms_copy(argc, argv, outv);
        outlet_anything(x->x->x_obj.ob_outlet, x->id.a_w.w_symbol, argc, outv);
    }
    SHADYLIB_ATOMS_FREEA(outv, argc);
}


static void proxy_float(t_proxy *x, t_floatarg f)
{
    if(x->id.a_type == A_FLOAT) {
        t_atom *outv;
        SHADYLIB_ATOMS_ALLOCA(outv, 2);
        SETFLOAT(outv + 1, f);
        outv[0] = x->id;
        outlet_list(x->x->x_obj.ob_outlet, &s_list, 2, outv);
        #if 2 >= SHADYLIB_LIST_NGETBYTE
            SHADYLIB_ATOMS_FREEA(outv, 2);
        #endif
    } else {
        t_atom fman;
        SETFLOAT(&fman, f);
        outlet_anything(x->x->x_obj.ob_outlet, x->id.a_w.w_symbol, 1, &fman);
    }

}


static t_class *prepender_class;
static t_class *proxy_class;

static void *prepender_new(t_symbol* SHADYLIB_UNUSED(s), int argc, t_atom *argv)
{
    t_proxy *iter;

    t_prepender *x = (t_prepender *)pd_new(prepender_class);
    if(argc > MAX_INLET) argc = MAX_INLET;
    if(!argc) {
        x->x_ninstance    = 0;
        x->x_outlet = outlet_new(&x->x_obj, NULL);
        x->x_proxies = NULL;
        SETSYMBOL(&x->x_id, &s_list);
        return (void *)x;
    }
    argc--;
    x->x_ninstance    = argc;
    x->x_proxies = getbytes(sizeof(t_proxy *) * argc);
    x->x_id = argv[0];
    for(int i = 0; i < argc; i++)
    {
        iter = (t_proxy *)pd_new(proxy_class);
        x->x_proxies[i] = iter;
        iter->x = x;        /* make x visible to the proxy inlets */
        iter->id = argv[i+1];
            /* the inlet we're going to create belongs to the object
               't_prepender' but the destination is the instance 'i'
               of the proxy class 't_proxy'                           */
        inlet_new(&x->x_obj, &iter->obj.ob_pd, 0,0);
    }
    x->x_outlet = outlet_new(&x->x_obj, NULL);

    return (void *)x;
}

static void prepender_free(t_prepender *x)
{
    if(!x->x_ninstance) return;
    for(int i = 0; i < x->x_ninstance; i++)
    {
        pd_free((t_pd *)x->x_proxies[i]);
    }
    freebytes(x->x_proxies, sizeof(t_proxy *) * x->x_ninstance);
}

void prepender_setup(void)
{
    prepender_class = class_new(gensym("prepender"), (t_newmethod)prepender_new,
        (t_method)prepender_free, sizeof(t_prepender), 0, A_GIMME, 0);
        /* a class for the proxy inlet: */
    proxy_class = class_new(gensym("shadylib_prepender_proxy"), NULL, NULL, sizeof(t_proxy),
        CLASS_PD|CLASS_NOINLET, A_NULL);

    class_addanything(proxy_class, proxy_any);
    class_addfloat(proxy_class, proxy_float);
    class_addlist(proxy_class, proxy_list);

    class_addfloat(prepender_class, prepender_float);
    class_addlist(prepender_class, prepender_list);
    class_addanything(prepender_class, prepender_any);

}
