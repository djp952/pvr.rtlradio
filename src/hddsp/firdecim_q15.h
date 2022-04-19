#pragma once

#include "defines.h"

typedef struct _firdecim_q15 {
	int16_t* taps;
	unsigned int ntaps;
	cint16_t* window;
	unsigned int idx;
} *firdecim_q15;

firdecim_q15 firdecim_q15_create(const float * taps, unsigned int ntaps);
void firdecim_q15_free(firdecim_q15);
void firdecim_q15_reset(firdecim_q15);
void fir_q15_execute(firdecim_q15 q, const cint16_t *x, cint16_t *y);
void halfband_q15_execute(firdecim_q15 q, const cint16_t *x, cint16_t *y);
