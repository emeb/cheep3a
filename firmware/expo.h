/*
 * expo.h - single-file header for exponential frequency calcs
 * 07-22-23 E. Brombaugh
 */

#ifndef __expo__
#define __expo__

#define LIN_SCALE 641
#define LIN_SHF 9
#define OCT_SHF 5
#define LOW_IDX 99
#define LUT_MASK 127
#define LUT_SHF 7

int16_t expo_lut[] = {
	16384,16472,16562,16652,16742,16833,16925,17016,
	17109,17202,17295,17389,17484,17578,17674,17770,
	17866,17963,18061,18159,18258,18357,18456,18557,
	18657,18759,18861,18963,19066,19169,19274,19378,
	19483,19589,19696,19803,19910,20018,20127,20236,
	20346,20457,20568,20679,20792,20905,21018,21132,
	21247,21362,21478,21595,21712,21830,21949,22068,
	22188,22308,22429,22551,22673,22797,22920,23045,
	23170,23296,23422,23549,23677,23806,23935,24065,
	24196,24327,24459,24592,24726,24860,24995,25131,
	25267,25404,25542,25681,25820,25961,26102,26243,
	26386,26529,26673,26818,26964,27110,27257,27405,
	27554,27704,27854,28005,28157,28310,28464,28619,
	28774,28930,29087,29245,29404,29564,29724,29886,
	30048,30211,30375,30540,30706,30873,31040,31209,
	31378,31549,31720,31892,32065,32239,32415,32591,
};

/*
 * exponential computation from 10-bit int
 * based on 48kHz sample rate and 10 octave range w/ 128 entries in LUT
 */
uint32_t expo_calc(uint16_t lin, uint16_t lfo_shf)
{
	uint32_t result;

	/* scale for 10 octaves @ 128 steps */
	result = (lin * LIN_SCALE) >> LIN_SHF;
	
	/* adjust for 10Hz bottom */
	result = result + LOW_IDX;
	
	/* get octave shift */
	lin = (result >> LUT_SHF) + OCT_SHF - lfo_shf;

	result = expo_lut[result&LUT_MASK] << lin;

	return result;
}

#endif
