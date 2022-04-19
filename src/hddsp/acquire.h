#pragma once

#include <complex.h>
#include <fftw3.h>
#include "firdecim_q15.h"

typedef struct
{
    struct input_t *input;
    firdecim_q15 filter_fm;
    firdecim_q15 filter_am;
    cint16_t in_buffer[FFTCP_FM * (ACQUIRE_SYMBOLS + 1)];
    fcomplex_t buffer[FFTCP_FM * (ACQUIRE_SYMBOLS + 1)];
    fcomplex_t sums[FFTCP_FM];
    fcomplex_t fftin[FFT_FM];
    fcomplex_t fftout[FFT_FM];
    float *shape;
    float shape_fm[FFTCP_FM];
    float shape_am[FFTCP_AM];
    fftwf_plan fft_plan_fm;
    fftwf_plan fft_plan_am;

    unsigned int idx;
    float prev_angle;
    fcomplex_t phase;
    int cfo;

    int mode;
    int fft;
    int fftcp;
    int cp;
} acquire_t;

void acquire_process(acquire_t *st);
void acquire_cfo_adjust(acquire_t *st, int cfo);
unsigned int acquire_push(acquire_t *st, cint16_t *buf, unsigned int length);
void acquire_reset(acquire_t *st);
void acquire_init(acquire_t *st, struct input_t *input);
void acquire_set_mode(acquire_t *st, int mode);
void acquire_free(acquire_t *st);
