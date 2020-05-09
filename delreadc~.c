#include "shadylib.h"
/*modified from pd source code */

/* ----------------------------- delreadc~ ----------------------------- */
static t_class *sigdelreadc_class;

typedef struct _sigdelreadc
{
    t_object x_obj;
    t_symbol *x_sym;
    t_float x_deltime;  /* delay in msec */
    int x_delsamps;     /* delay in samples */
    t_float x_sr;       /* samples per msec */
    t_float x_n;        /* vector size */
    int x_zerodel;      /* 0 or vecsize depending on read/write order */
} t_sigdelreadc;

static void sigdelreadc_float(t_sigdelreadc *x, t_float f);

static void *sigdelreadc_new(t_symbol *s, t_floatarg f)
{
    t_sigdelreadc *x = (t_sigdelreadc *)pd_new(sigdelreadc_class);
    x->x_sym = s;
    x->x_sr = 1;
    x->x_n = 1;
    x->x_zerodel = 0;
    sigdelreadc_float(x, f);
    outlet_new(&x->x_obj, &s_signal);
    return (x);
}

static void sigdelreadc_float(t_sigdelreadc *x, t_float f)
{
    t_sigdelwritec *delwriter =
        (t_sigdelwritec *)pd_findbyclass(x->x_sym, sigdelwritec_class);
    x->x_deltime = f;
    if (delwriter)
    {
        x->x_delsamps = (int)(0.5 + x->x_sr * x->x_deltime)
            + x->x_n - x->x_zerodel;
        if (x->x_delsamps < x->x_n) x->x_delsamps = x->x_n;
        else if (x->x_delsamps > delwriter->x_cspace.c_n)
            x->x_delsamps = delwriter->x_cspace.c_n;
    }
}

static t_int *sigdelreadc_perform(t_int *w)
{
    t_sample *out = (t_sample *)(w[1]);
    shadylib_t_delwritectl *c = (shadylib_t_delwritectl *)(w[2]);
    int delsamps = *(int *)(w[3]);
    int n = (int)(w[4]);
    int phase = c->c_phase - delsamps, nsamps = c->c_n;
    t_sample *vp = c->c_vec, *bp, *ep = vp + (c->c_n + XTRASAMPS);
    if (phase < 0) phase += nsamps;
    bp = vp + phase;

    while (n--)
    {
        *out++ = *bp++;
        if (bp == ep) bp -= nsamps;
    }
    return (w+5);
}

static void sigdelreadc_dsp(t_sigdelreadc *x, t_signal **sp)
{
    t_sigdelwritec *delwriter =
        (t_sigdelwritec *)pd_findbyclass(x->x_sym, sigdelwritec_class);
    x->x_sr = sp[0]->s_sr * 0.001;
    x->x_n = sp[0]->s_n;
    if (delwriter)
    {
        sigdelwritec_updatesr(delwriter, sp[0]->s_sr);
        sigdelwritec_checkvecsize(delwriter, sp[0]->s_n);
        x->x_zerodel = (delwriter->x_sortno == ugen_getsortno() ?
            0 : delwriter->x_vecsize);
        sigdelreadc_float(x, x->x_deltime);
        dsp_add(sigdelreadc_perform, 4,
            sp[0]->s_vec, &delwriter->x_cspace, &x->x_delsamps, sp[0]->s_n);
    }
    else if (*x->x_sym->s_name)
        error("delreadc~: %s: no such delwritec~",x->x_sym->s_name);
}

void delreadc_tilde_setup(void)
{
    sigdelreadc_class = class_new(gensym("delreadc~"),
        (t_newmethod)sigdelreadc_new, 0,
        sizeof(t_sigdelreadc), 0, A_DEFSYM, A_DEFFLOAT, 0);
    class_addmethod(sigdelreadc_class, (t_method)sigdelreadc_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addfloat(sigdelreadc_class, (t_method)sigdelreadc_float);
}