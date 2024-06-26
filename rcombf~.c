/* recirculating comb filter */
#include "shadylib.h"
#include <string.h>

static t_class *rcombf_class;
/*modified from pd source */

typedef struct _rcombf
{
    t_object x_obj;
    t_float x_f;
    t_float x_sr;
    t_float x_ttime;  /* delay line length msec */
    int c_n; /* same in samples */
    int c_phase; /* current write point */
    int norm; /* whether to normalize or not */
    t_sample *c_vec; /* ring buffer for delay line */
} t_rcombf;

static void rcombf_updatesr (t_rcombf *x, t_float sr) /* added by Mathieu Bouchard */
{
    int nsamps = x->x_ttime * sr * (t_float)(0.001f);
    if (nsamps < 1) nsamps = 1;
    nsamps += ((- nsamps) & (SHADYLIB_SAMPBLK - 1));
    nsamps += SHADYLIB_DEFDELVS;
    if (x->c_n != nsamps) {
      x->c_vec = (t_sample *)resizebytes(x->c_vec,
        (x->c_n + SHADYLIB_XTRASAMPS) * sizeof(t_sample),
        (nsamps + SHADYLIB_XTRASAMPS) * sizeof(t_sample));
      x->c_n = nsamps;
      x->c_phase = SHADYLIB_XTRASAMPS;
    }
}

static t_int *rcombf_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *time = (t_sample *)(w[2]);
    t_sample *fb = (t_sample *)(w[3]);
    t_sample *norm = (t_sample *)(w[4]);
    t_sample *out = (t_sample *)(w[5]);
    t_rcombf *x = (t_rcombf *)(w[6]);
    int n = (int)(w[7]);

    int phase = x->c_phase, nsamps = x->c_n;
    int idelsamps;
    t_sample limit = nsamps - 3;
    t_sample *vp = x->c_vec, *bp, *wp = vp + phase,
        *ep = vp + (nsamps + SHADYLIB_XTRASAMPS);
    phase += n;
    while (n--)
    {
        t_sample f = *in++, inorm = *norm++;
        if (PD_BIGORSMALL(f))
            f = 0;
        f *= shadylib_clamp(inorm, -1.0, 1.0);
        t_sample delsamps = x->x_sr * (*time++), frac;

        t_sample a, b, c, d, cminusb;
        if (!(delsamps >= 1.f))    /* too small or NAN */
            delsamps = 1.f;
        if (delsamps > limit)           /* too big */
            delsamps = limit;
        idelsamps = delsamps;
        frac = delsamps - (t_sample)idelsamps;
        bp = wp - idelsamps;
        if (bp < vp + 4) bp += nsamps;
        d = bp[-3];
        c = bp[-2];
        b = bp[-1];
        a = bp[0];
        cminusb = c-b;
       #ifdef FP_FAST_FMAF
       delsamps = fmaf(frac, (
            cminusb - 0.1666667f * (1.-frac) * fmaf(
                (fmaf(-3.0f, cminusb, d - a)), frac, fmaf(-3.0, b, fmaf(2.0, a, d))
            )
        ), b);
        #else
        delsamps = b + frac * (
            cminusb - 0.1666667f * (1.-frac) * (
                (d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
            )
        );
        #endif
        a = *fb++;
        a = shadylib_clamp(a, -0x1.fffffp-1, 0x1.fffffp-1);
        #ifdef FP_FAST_FMAF
        c = fmaf(delsamps, a, f);
        #else
        c = f + delsamps*a;
        #endif
        *wp++ = c;
        *out++ = c;
        if (wp == ep)
        {
            vp[0] = ep[-4];
            vp[1] = ep[-3];
            vp[2] = ep[-2];
            vp[3] = ep[-1];
            wp = vp + SHADYLIB_XTRASAMPS;
            phase -= nsamps;
        }
    }
    x->c_phase = phase;
    return (w+8);
}

static t_int *nrcombf_perform(t_int *w)
{
    t_sample *in = (t_sample *)(w[1]);
    t_sample *time = (t_sample *)(w[2]);
    t_sample *fb = (t_sample *)(w[3]);
    t_sample *out = (t_sample *)(w[4]);
    t_rcombf *x = (t_rcombf *)(w[5]);
    int n = (int)(w[6]);

    int phase = x->c_phase, nsamps = x->c_n;
    int idelsamps;
    t_sample limit = nsamps - 3;
    t_sample *vp = x->c_vec, *bp, *wp = vp + phase,
        *ep = vp + (nsamps + SHADYLIB_XTRASAMPS);
    t_sample f, feedback, delsamps, a, b, c, d, cminusb, frac;
    phase += n;
    while (n--)
    {
        f = *in++;
        if (PD_BIGORSMALL(f))
            f = 0;
        feedback = *fb++;
        feedback = shadylib_clamp(feedback, -0x1.fffffp-1, 0x1.fffffp-1);
        f *= 1.f - shadylib_pdfloat_abs(feedback);
        delsamps = x->x_sr * (*time++);

        if (!(delsamps >= 1.f))    /* too small or NAN */
            delsamps = 1.f;
        if (delsamps > limit)           /* too big */
            delsamps = limit;
        idelsamps = delsamps;
        frac = delsamps - (t_sample)idelsamps;
        bp = wp - idelsamps;
        if (bp < vp + 4) bp += nsamps;
        d = bp[-3];
        c = bp[-2];
        b = bp[-1];
        a = bp[0];
        cminusb = c-b;
        #ifdef FP_FAST_FMAF
       delsamps = fmaf(frac, (
            cminusb - 0.1666667f * (1.-frac) * fmaf(
                (fmaf(-3.0f, cminusb, d - a)), frac, fmaf(-3.0, b, fmaf(2.0, a, d))
            )
        ), b);
        #else
        delsamps = b + frac * (
            cminusb - 0.1666667f * (1.-frac) * (
                (d - a - 3.0f * cminusb) * frac + (d + 2.0f*a - 3.0f*b)
            )
        );
        #endif
        #ifdef FP_FAST_FMAF
        c = fmaf(delsamps, feedback, f);
        #else
        c = f + delsamps*feedback;
        #endif
        *wp++ = c;
        *out++ = c;
        if (wp == ep)
        {
            vp[0] = ep[-4];
            vp[1] = ep[-3];
            vp[2] = ep[-2];
            vp[3] = ep[-1];
            wp = vp + SHADYLIB_XTRASAMPS;
            phase -= nsamps;
        }
    }
    x->c_phase = phase;
    return (w+7);
}

static void rcombf_dsp(t_rcombf *x, t_signal **sp)
{
    if(x->norm)
        dsp_add(nrcombf_perform, 6, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,
            sp[3]->s_vec, x, sp[0]->s_n);
    else dsp_add(rcombf_perform, 7, sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec,
        sp[3]->s_vec, sp[4]->s_vec, x, sp[0]->s_n);
       x->x_sr = sp[0]->s_sr * 0.001;
    rcombf_updatesr(x, sp[0]->s_sr);
}

static void *rcombf_new(t_symbol* SHADYLIB_UNUSED(s), int argc, t_atom *argv)
{
    t_float time = 1000.f;
    t_float size = 0.f;
    t_float fb = 0.f;
    t_symbol *sarg;
    int norm = 0, i = 0, which = 0;
    t_rcombf *x = (t_rcombf *)pd_new(rcombf_class);
    for(; i < argc; i++)
        if (argv[i].a_type == A_FLOAT) {
            switch (which) {
                case 0:
                    time = atom_getfloatarg(i, argc, argv);
                    if (time < 0.f) time = 0.f;
                    break;
                case 1:
                    fb = atom_getfloatarg(i, argc, argv);
                    break;
                case 2:
                    size = atom_getfloatarg(i, argc, argv);
                default:;
            }
            which++;
        } else {
            sarg = atom_getsymbolarg(i, argc, argv);
            if (sarg == gensym("-n")) norm = 1;
            else if(sarg == gensym("-l")) {
                i++;
                if(i < argc)
                    size = atom_getfloatarg(i, argc, argv);
            }
        }
    signalinlet_new(&x->x_obj, time);
    signalinlet_new(&x->x_obj, fb);
    if(size <= 0.f) size = time;
    x->x_ttime = size;
    if(!norm)
        signalinlet_new(&x->x_obj, 1.f);
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0.f;
    x->c_n = 0;
    x->c_vec = getbytes(SHADYLIB_XTRASAMPS * sizeof(t_sample));
    memset((char *)(x->c_vec), 0,
        sizeof(t_sample)*(SHADYLIB_XTRASAMPS));
    x->norm = norm;
    return(x);
}

static void rcombf_clear(t_rcombf *x) {
    memset((char *)(x->c_vec), 0,
        sizeof(t_sample)*(x->c_n + SHADYLIB_XTRASAMPS));
}

static void rcombf_free(t_rcombf *x)
{
    freebytes(x->c_vec,
        (x->c_n + SHADYLIB_XTRASAMPS) * sizeof(t_sample));
}

void rcombf_tilde_setup(void)
{
    rcombf_class = class_new(gensym("rcombf~"),
        (t_newmethod)rcombf_new, (t_method)rcombf_free,
        sizeof(t_rcombf), 0, A_GIMME, 0);
    CLASS_MAINSIGNALIN(rcombf_class, t_rcombf, x_f);
    class_addmethod(rcombf_class, (t_method)rcombf_dsp,
        gensym("dsp"), A_CANT, 0);
    class_addmethod(rcombf_class, (t_method)rcombf_clear,
        gensym("clear"), 0);
}
