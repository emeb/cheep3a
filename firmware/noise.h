/*
 * noise.h - single-file include for noise generation
 * 07-23-23 E. Brombaugh
 */

#ifndef __noise__
#define __noise__

/* White Noise Generator State */
#define NOISE_BITS 16
#define NOISE_MASK ((1<<NOISE_BITS)-1)
#define NOISE_POLY_TAP0 31
#define NOISE_POLY_TAP1 21
#define NOISE_POLY_TAP2 1
#define NOISE_POLY_TAP3 0

uint32_t wht_noise_lfsr;

/* Pink noise states */
int16_t pnk_state[16], pnk_cntr;

void init_noise(void)
{
	/* init white noise generator */
	wht_noise_lfsr = 1;
	
	/* init pink noise generator */
	pnk_cntr = 0;
}

/* 32-bit LFSR noise generator */
uint16_t PRN(uint32_t *lfsr)
{
	uint8_t bit;
	uint32_t new_data;
	
	for(bit=0;bit<NOISE_BITS;bit++)
	{
		new_data = ((*lfsr>>NOISE_POLY_TAP0) ^
					(*lfsr>>NOISE_POLY_TAP1) ^
					(*lfsr>>NOISE_POLY_TAP2) ^
					(*lfsr>>NOISE_POLY_TAP3));
		*lfsr = (*lfsr<<1) | (new_data&1);
	}
	
	return *lfsr&NOISE_MASK;
}

/* plain white noise */
int16_t wht_noise(void)
{
	return PRN(&wht_noise_lfsr);
}

/* Pink noise needs white input */
int16_t pnk_noise(int16_t wht)
{
	uint16_t bit;
	int32_t sum;
	
	/* store new white value in overlapping states */
	for(bit=0;bit<15;bit++)
		if(((pnk_cntr>>bit)&1) == 1)
			break;
	pnk_state[bit] = wht;
	pnk_cntr++;
	
	/* sum overlapping states */
	sum = 0;
	for(bit=0;bit<16;bit++)
		sum += pnk_state[bit];
	
	return sum >> 4;
}

#if 0
/* Clocked noise */
uint16_t clk_noise(void)
{
	/* update NCO */
	clk_phs += clk_frq;
	
	/* overflow? */
	if(clk_phs & 0x40000)
	{
		/* clear carry */
		clk_phs &= 0x3ffff;
		
		/* Update LFSR */
		PRN(&clk_lfsr);
	}
	return clk_lfsr&NOISE_MASK;
}
#endif

#endif
