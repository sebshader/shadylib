#include "shadylib.h"
#include <math.h>

static t_class *bpbuzz_class;
/* this algorithm is mainly from supercollider
should be modified for 16-bit ints;
*/

typedef struct _bpbuzz {
    t_object x_obj;
    t_float x_f;
    float x_duty;
    float x_oduty;
    double phase;
    double dphase;
    double d_conv;
    double x_conv;
    float max;
} t_bpbuzz;

static void bpbuzz_phase(t_bpbuzz *x, t_float f) {
    f *= .5f;
    x->phase = f - floorf(f);
}

static void *bpbuzz_new(t_symbol* SHADYLIB_UNUSED(s), int argc, t_atom *argv) {
    t_bpbuzz *x = (t_bpbuzz *)pd_new(bpbuzz_class);
    float oduty = argc ? atom_getfloatarg(0, argc, argv) : 0.5f;
    x->phase = 0.0f;

    pd_float(
        (t_pd *)inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal),
            oduty);
    x->x_duty = x->x_oduty = 0.495 - 0.49*shadylib_clamp(oduty, 0.0, 1.0);
    inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_float, gensym("phase"));
    outlet_new(&x->x_obj, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    outlet_new(&x->x_obj, &s_signal);
    x->x_f = 0;
    return (x);
}

static t_int *bpbuzz_perform(t_int *w) {
    t_bpbuzz *x = (t_bpbuzz *)(w[1]);
    t_sample *in = (t_sample *)(w[2]);
    t_sample *in2 = (t_sample *)(w[3]);
    t_sample *out1 = (t_sample *)(w[4]);
    t_sample *out2 = (t_sample *)(w[5]);
    t_sample *out3 = (t_sample *)(w[6]);
    int n = (int)(w[7]);
    double conv = x->x_conv, dconv = x->d_conv;
    double phase = x->phase, dphase = x->dphase;
    double rat, frat, res1, res2, res3, res4, fread;
    double freq, final, max = x->max, duty = x->x_duty, oduty = x->x_oduty;
    uint32_t tabrd, tabrd2, n2;

    for(int i=0; i < n; i++) {
        freq = shadylib_pdfloat_abs(*in++);
        freq = shadylib_min(freq, max);
        fread = phase*SHADYLIB_BUZZSIZE;
        tabrd = fread;
        tabrd2 = tabrd + 1;
        res1 = shadylib_cosectbl[tabrd];
        res2 = shadylib_cosectbl[tabrd2];
        res3 = fread - tabrd;
        if(res1 == SHADYLIB_BADVAL || res2 == SHADYLIB_BADVAL) {
            res1 = shadylib_sintbl[tabrd];
            res2 = shadylib_sintbl[tabrd2];
            #ifdef FP_FAST_FMA
            res2 = fma(res2 - res1, res3, res1);
            #else
            res2 = res1 + (res2 - res1)*res3;
            #endif
            if(fabs(res2)  < 0.0005) {
                final = 1.0;

                rat = shadylib_clamp(max/freq, 1.0, SHADYLIB_MAXHARM);
                n2 = rat;
                n2 *= 2;

                goto gotfinal;
            } else res2 = 1.0/res2;
        } else {
            #ifdef FP_FAST_FMA
            res2 = fma(res2 - res1, res3, res1);
            #else
            res2 = res1 + (res2 - res1)*res3;
            #endif
        }
        rat = max/freq - 1.0;
        rat = shadylib_clamp(rat, 1.0, SHADYLIB_MAXHARM);
        n2 = rat;
        frat = rat - n2;
        n2 *= 2;
        #ifdef FP_FAST_FMA
        res4 = fma(n2, fread, fread);
        #else
        res4 = fread*(n2 + 1);
        #endif
        tabrd = res4;
        res1 = res4 - tabrd;
        tabrd = tabrd & (SHADYLIB_BUZZSIZE - 1);
        res3 = shadylib_sintbl[tabrd];
        tabrd++;
        #ifdef FP_FAST_FMA
        res3 = fma(shadylib_sintbl[tabrd] - res3, res1, res3);
        res3 = fma(res3, res2, -1)/n2;
        res1 = fma(fread, 2, res4);
        #else
        res3 = res3 + (shadylib_sintbl[tabrd] - res3)*res1;
        res3 = (res3*res2 - 1.0)/n2;
        res1 = res4 + fread*2.0;
        #endif
        /* crossfade to last harmonic */
        tabrd = res1;
        res4 = res1 - tabrd;
        tabrd = tabrd & (SHADYLIB_BUZZSIZE - 1);
        res1 = shadylib_sintbl[tabrd];
        tabrd++;

        #ifdef FP_FAST_FMA
        res1 = fma(shadylib_sintbl[tabrd] - res1, res4, res1);
        res1 = fma(res1, res2, -1)/(n2 + 2);
        final = fma(res3, 1-frat, res1*frat);
        #else
        res1 = res1 + (shadylib_sintbl[tabrd] - res1)*res4;
        res1 = (res1*res2 - 1.0)/(n2 + 2);
        final = res3*(1.0 - frat) + res1*(frat);
        #endif

gotfinal:
        if(freq == 0.0) {
            res3 = 0.0;
            rat = phase + oduty + dphase;
            goto gotfinal2;
        }
        rat = max*(1.0 - ((1.0 - 2.0*dconv)*dconv))/freq - 1.0;
        rat = shadylib_clamp(rat, 1.0, SHADYLIB_MAXHARM);
        n2 = rat;
        frat = rat - n2;
        n2 *= 2;

        fread = rat = phase + oduty + dphase;
        // first wrap between 0 and 1 so 32 bit
        // ints can take maxharm
        tabrd = fread;
        fread = fread - tabrd;
        fread *= SHADYLIB_BUZZSIZE;
        tabrd = fread;
        tabrd = tabrd & (SHADYLIB_BUZZSIZE - 1);
        res3 = fread - tabrd;
        tabrd2 = tabrd + 1;
        res1 = shadylib_cosectbl[tabrd];
        res2 = shadylib_cosectbl[tabrd2];
        if(res1 == SHADYLIB_BADVAL || res2 == SHADYLIB_BADVAL) {
            res1 = shadylib_sintbl[tabrd];
            res2 = shadylib_sintbl[tabrd2];
            res2 = res1 + (res2 - res1)*res3;
            if(fabs(res2)  < 0.0005) {
                res3 = 1.0;
                goto gotfinal2;
            } else res2 = 1.0/res2;
        } else {
            #ifdef FP_FAST_FMA
            res2 = fma(res2 - res1, res3, res1);
            #else
            res2 = res1 + (res2 - res1)*res3;
            #endif
        }
        #ifdef FP_FAST_FMA
        res4 = fma(n2, fread, fread);
        #else
        res4 = fread*(n2 + 1);
        #endif
        tabrd = res4;
        res1 = res4 - tabrd;
        tabrd = tabrd & (SHADYLIB_BUZZSIZE - 1);
        res3 = shadylib_sintbl[tabrd];
        tabrd++;
        #ifdef FP_FAST_FMA
        res3 = fma(shadylib_sintbl[tabrd] - res3, res1, res3);
        res3 = fma(res3, res2, -1)/n2;
        res1 = fma(fread, 2, res4);
        #else
        res3 = res3 + (shadylib_sintbl[tabrd] - res3)*res1;
        res3 = (res3*res2 - 1.0)/n2;
        res1 = res4 + fread*2.0;
        #endif
        /* crossfade to last harmonic */
        tabrd = res1;
        res4 = res1 - tabrd;
        tabrd = tabrd & (SHADYLIB_BUZZSIZE - 1);
        res1 = shadylib_sintbl[tabrd];
        tabrd++;

        #ifdef FP_FAST_FMA
        res1 = fma(shadylib_sintbl[tabrd] - res1, res4, res1);
        res1 = fma(res1, res2, -1)/(n2 + 2);
        res3 = fma(res3, 1-frat, res1*frat);
        #else
        res1 = res1 + (shadylib_sintbl[tabrd] - res1)*res4;
        res1 = (res1*res2 - 1.0)/(n2 + 2);
        res3 = res3*(1.0 - frat) + res1*(frat);
        #endif

gotfinal2:
        n2 = rat*2.0;
        res2 = freq*conv;
        phase += res2;
        dphase += res2*dconv;
        res1 = phase + oduty + dphase;
        tabrd = res1*2.0;
        /* calculate phase offset for bp */
        if(tabrd > n2) {
            dphase = dphase - duty + oduty;
            oduty = duty;
            duty = shadylib_clamp(*in2, 0.0, 1.0);
            #ifdef FP_FAST_FMA
            duty = fma(duty, -0.49, .495);
            #else
            duty = 0.495 - 0.49*duty;
            #endif

            dconv = duty - oduty - dphase;
            dconv = (dconv)/(0.5 - dconv);

        }
        in2++;
        n2 = phase;
        phase = phase - n2;
        *out1++ = (final - res3);
        *out2++ = final;
        *out3++ = phase;
    }
    x->phase = phase, x->dphase = dphase;
    x->d_conv = dconv;
    x->x_duty = duty, x->x_oduty = oduty;
    return(w + 8);
}


static void bpbuzz_dsp(t_bpbuzz *x, t_signal **sp)
{
    x->x_conv = 0.5/sp[0]->s_sr;
    x->max = sp[0]->s_sr*0.5;
    dsp_add(bpbuzz_perform, 7, x, sp[0]->s_vec, sp[1]->s_vec,
            sp[2]->s_vec, sp[3]->s_vec, sp[4]->s_vec, sp[0]->s_n);
}

void bpbuzz_tilde_setup(void)
{
    bpbuzz_class = class_new(gensym("bpbuzz~"), (t_newmethod)bpbuzz_new, 0,
        sizeof(t_bpbuzz), 0, A_GIMME, 0);
    class_addmethod(bpbuzz_class, (t_method)bpbuzz_dsp, gensym("dsp"), A_CANT, 0);
    CLASS_MAINSIGNALIN(bpbuzz_class, t_bpbuzz, x_f);
    class_addmethod(bpbuzz_class, (t_method)bpbuzz_phase,
        gensym("phase"), A_FLOAT, 0);
    class_setfreefn(bpbuzz_class, shadylib_freebuzz);
    shadylib_makebuzz();
}
