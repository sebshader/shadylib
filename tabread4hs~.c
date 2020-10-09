// tabosc4hs~
// can replace with tabosc4~
// most of this code comes from pd. just the interpolation shematic is diferent.

/* 
This software is copyrighted by Miller Puckette and others.  The following
terms (the "Standard Improved BSD License") apply to all files associated with
the software unless explicitly disclaimed in individual files:

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above  
   copyright notice, this list of conditions and the following 
   disclaimer in the documentation and/or other materials provided
   with the distribution.
3. The name of the author may not be used to endorse or promote
   products derived from this software without specific prior 
   written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,   
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "m_pd.h"

static t_class *tabread4hs_tilde_class;

typedef struct _tabread4hs_tilde {
    t_object x_obj;
    int x_npoints;
    t_word *x_vec;
    t_symbol *x_arrayname;
    t_float x_f;
    t_float x_onset;
} t_tabread4hs_tilde;

void *tabread4hs_tilde_new(t_symbol *s) {
    t_tabread4hs_tilde *x = (t_tabread4hs_tilde *)pd_new(tabread4hs_tilde_class);
    x->x_arrayname = s;
    x->x_vec = 0;
    outlet_new(&x->x_obj, &s_signal);
	inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);
	x->x_f = 0;
    x->x_onset = 0;
    return (x);
}

t_int *tabread4hs_tilde_perform(t_int *w) {
    t_tabread4hs_tilde *x = (t_tabread4hs_tilde *)(w[1]);
    t_sample *in1 = (t_sample *)(w[2]);
    t_sample *in2 = (t_sample *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    int n = (int)(w[5]);    
    int maxindex;
    t_word *buf = x->x_vec, *wp;
    double onset = x->x_onset;
    int i;
    
    maxindex = x->x_npoints - 3;

    if (!buf) goto zero;

#if 0       /* test for spam -- I'm not ready to deal with this */
    for (i = 0,  xmax = 0, xmin = maxindex,  fp = in1; i < n; i++,  fp++)
    {
        t_sample f = *in1;
        if (f < xmin) xmin = f;
        else if (f > xmax) xmax = f;
    }
    if (xmax < xmin + x->c_maxextent) xmax = xmin + x->c_maxextent;
    for (i = 0, splitlo = xmin+ x->c_maxextent, splithi = xmax - x->c_maxextent,
        fp = in1; i < n; i++,  fp++)
    {
        t_sample f = *in1;
        if (f > splitlo && f < splithi) goto zero;
    }
#endif

    for (i = 0; i < n; i++) {
        double findex = *in1++;
        findex += *in2++;
        int index = findex;
        t_sample frac, a, b, c, d;
        double a3,a1,a2;
        if (index < 1)
            index = 1, frac = 0;
        else if (index > maxindex)
            index = maxindex, frac = 1;
        else frac = findex - index;
        wp = buf + index;
        a = wp[-1].w_float;
        b = wp[0].w_float;
        c = wp[1].w_float;
        d = wp[2].w_float;
        
		// 4-point, 3rd-order Hermite (x-form)
		a1 = 0.5f * (c - a);
		#ifdef FP_FAST_FMA
		a2 =  fma(2.f, c, fma(0.5f, d, fma(2.5, b, a)));
		a3 = fma(0.5f, (d - a), 1.5f * (b - c));
		*out++ =  fma(fma(fma(a3, frac, a2), frac, a1), frac, b);
		#else
		a2 = a - 2.5 * b + 2.f * c - 0.5f * d;
		a3 = 0.5f * (d - a) + 1.5f * (b - c);
		*out++ =  ((a3 * frac + a2) * frac + a1) * frac + b;
		#endif
    }
    return (w+6);
 zero:
    while (n--) *out++ = 0;

    return (w+6);
}

void tabread4hs_tilde_set(t_tabread4hs_tilde *x, t_symbol *s) {
    t_garray *a;
    
    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class))) {
        if (*s->s_name)
            pd_error(x, "tabread4hs~: %s: no such array", x->x_arrayname->s_name);
        x->x_vec = 0;
    } else if (!garray_getfloatwords(a, &x->x_npoints, &x->x_vec)) {
        pd_error(x, "%s: bad template for tabread4hs~", x->x_arrayname->s_name);
        x->x_vec = 0;
    } else garray_usedindsp(a);
}

void tabread4hs_tilde_dsp(t_tabread4hs_tilde *x, t_signal **sp) {
    tabread4hs_tilde_set(x, x->x_arrayname);

    dsp_add(tabread4hs_tilde_perform, 5, x,
        sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);

}

void tabread4hs_tilde_setup(void) {
    tabread4hs_tilde_class = class_new(gensym("tabread4hs~"),
        (t_newmethod)tabread4hs_tilde_new, 0,
        sizeof(t_tabread4hs_tilde), 0, A_DEFSYM, 0);
    CLASS_MAINSIGNALIN(tabread4hs_tilde_class, t_tabread4hs_tilde, x_f);
    class_addmethod(tabread4hs_tilde_class, (t_method)tabread4hs_tilde_dsp,
        gensym("dsp"), 0);
    class_addmethod(tabread4hs_tilde_class, (t_method)tabread4hs_tilde_set,
        gensym("set"), A_SYMBOL, 0);
}