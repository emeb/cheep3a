/*
 * Cheep3a firmware
 * 07-25-2023 E. Brombaugh
 */

#include "ch32v003fun.h"
#include <stdio.h>
#include <string.h>

// Systick IRQ
#include "systick.h"

// EEPROM emulation for mode storage
#include "eeprom.h"

// LED driver
#include "led.h"

// Sync input driver
#include "sync.h"

// prototype for the DAC update function used in the driver
// must come prior to the driver include
void dac_update(uint16_t *buffer);

// the SPI DAC driver
#include "spi_dac.h"

// ADC driver
#include "adc.h"

// Expo
#include "expo.h"

// Noise
#include "noise.h"

// this table contains a 512-sample 16-bit sine waveform
#include "Sine16bit.h"

/* audio state */
uint8_t mode, psync;
int32_t osc_phs[2], osc_frq[2];
int16_t clk_nz[2];
int64_t fm_phs;
uint16_t fm_index, fm_ratio, live_fm_ratio;

/*
 * hysteresis & reduction
 */
void ratio_hyst(uint16_t *old, uint16_t in, uint8_t shift)
{
	int16_t diff, guard = (1<<(shift-2));
	
	/* Special case - don't guardband at endpoints */
	if((in == 0) || (in == 0x3FF))
	{
		*old = in >> shift;
		return;
	}
	
	/* find abs val of diff */
	diff = (int16_t)(*old << shift) - (int16_t)in;
	if(diff<0)
		diff = -diff;
	
	/* exceeds guardband ? */
	if(diff > guard)
	{
		/* yes so update prev */
		*old  = in >> shift;
	}
}

/*
 * 32-bit phase to 16-bit sine with quarter-cycle LUT
 */
#define SINE_LUT_MASK ((1<<SINE_LUT_BITS)-1)
int16_t SineWave(uint32_t Phs)
{
	int16_t result;
	int16_t sgn = (Phs & 0x80000000) ? 1 : 0;
	uint32_t dir = (Phs & 0x40000000) ? SINE_LUT_MASK : 0;
	
	Phs = ((Phs>>(30-SINE_LUT_BITS)) & SINE_LUT_MASK) ^ dir;
	result = Sine16bit[Phs];
	
	/* sign inversion w/o multiply */
	return sgn ? ~result + 1 : result ;
}

/*
 * DAC buffer updates - called at IRQ time
 */
void dac_update(uint16_t *buffer)
{
	uint8_t i;
	int16_t phs, amp, pw1, pw2;
	int32_t tphs, tmp_phs;
	
	// Get sync input
	if(sync_re())
	{
		osc_phs[0] = osc_phs[1] = 0;
	}
	
	// fill the buffer with stereo data
	switch(mode)
	{
		/* sine oscillators with separate pitch */
		case 1:	// Normal range
		case 2:	// LFO range
			osc_frq[0] = expo_calc(1023-adc_buffer[0], mode & 1 ? 0 : 5);
			osc_frq[1] = expo_calc(1023-adc_buffer[1], mode & 1 ? 0 : 5);
			amp = 511 - adc_buffer[2];
			for(i=0;i<DACBUFSZ/2;i+=2)
			{
				// right chl
				*buffer++ = (amp * SineWave(osc_phs[0]))>>9;
				osc_phs[0] += osc_frq[0];
				
				// left chl
				*buffer++ = (amp * SineWave(osc_phs[1]))>>9;
				osc_phs[1] += osc_frq[1];
			}
			break;
		
		/* sine oscillator with two outputs, phase / amp control */
		case 3:	// Normal range
			osc_frq[0] = expo_calc(1023-adc_buffer[0], 0);
			phs = (1023 - adc_buffer[1])>>2;
			amp = 511 - adc_buffer[2];
			for(i=0;i<DACBUFSZ/2;i+=2)
			{
				// right chl
				*buffer++ = (amp * SineWave(osc_phs[0]))>>9;
				
				// left chl
				*buffer++ = (amp * SineWave(osc_phs[0] + (phs<<22)))>>9;
				
				osc_phs[0] += osc_frq[0];
			}
			break;
		
		/* square oscillator with two outputs, PWM control */
		case 4:
			osc_frq[0] = expo_calc(1023-adc_buffer[0], 0);
			pw1 = (511 - adc_buffer[1]);
			pw1 = pw1 > 480 ? 480 : pw1;
			pw1 = pw1 < -480 ? -480 : pw1;
			pw2 = (511 - adc_buffer[2]);
			pw2 = pw2 > 480 ? 480 : pw2;
			pw2 = pw2 < -480 ? -480 : pw2;
			for(i=0;i<DACBUFSZ/2;i+=2)
			{
				// right chl
				*buffer++ = (osc_phs[0]>>22) > pw1 ? -32767 : 32767;
				
				// left chl
				*buffer++ = (osc_phs[0]>>22) > pw2 ? -32767 : 32767;
				
				osc_phs[0] += osc_frq[0];
			}
			break;

		/* 2-op FM */
		case 5:
			osc_frq[0] = expo_calc(1023-adc_buffer[0], 0);
			fm_index = (1023 - adc_buffer[1])<<2;
			ratio_hyst(&fm_ratio, (1023 - adc_buffer[2]), 4);
			for(i=0;i<DACBUFSZ/2;i+=2)
			{
				/* compute FM */
				tmp_phs = fm_phs >> 3;
				if((tmp_phs & 0xFF000000)==0)
					live_fm_ratio = fm_ratio;	// to avoid ratio change glitches
				tmp_phs = (tmp_phs * live_fm_ratio);
				int16_t mod = SineWave(tmp_phs);
				tmp_phs = (mod * fm_index)<<4;
				tmp_phs = tmp_phs + fm_phs;
				
				/* send to DAC */
				*buffer++ = mod;
				*buffer++ = SineWave(tmp_phs);
				
				/* update phase */
				fm_phs += osc_frq[0];
			}
			break;
		
		/* clocked noise with two separate oscs */
		case 6:
			osc_frq[0] = expo_calc(1023-adc_buffer[0], 0);
			osc_frq[1] = expo_calc(1023-adc_buffer[1], 0);
			for(i=0;i<DACBUFSZ/2;i+=2)
			{
				uint16_t wht = wht_noise();
				
				// right chl
				tphs = osc_phs[0];
				osc_phs[0] += osc_frq[0];
				if(((tphs & 0x80000000) == 0x80000000) &&
					((osc_phs[0] & 0x80000000) == 0x00000000))
					clk_nz[0] = wht;
				*buffer++ = clk_nz[0];
				
				// left chl
				tphs = osc_phs[1];
				osc_phs[1] += osc_frq[1];
				if(((tphs & 0x80000000) == 0x80000000) &&
					((osc_phs[1] & 0x80000000) == 0x00000000))
					clk_nz[1] = wht;
				*buffer++ = clk_nz[1];
			}
			break;
		
		/* white and pink noise */
		case 7:
			for(i=0;i<DACBUFSZ/2;i+=2)
			{
				uint16_t wht = wht_noise();
				
				// right chl
				*buffer++ = (int16_t)wht;
				
				// left chl
				*buffer++ = pnk_noise(wht);
			}
			break;
		
		/* do nothing */
		default:
			memset(buffer, 0, DACBUFSZ * sizeof(int16_t));
			break;
	}
}

/* build version in simple format */
const char *fwVersionStr = "V1.0";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/*
 * Start here
 */
int main()
{
	uint8_t eepend = 0;
	uint32_t eegoal;
	
	// init audio state
	mode = 0;
	fm_phs = 0;
	osc_phs[0] = 0;
	osc_phs[1] = 0;
	osc_frq[0] = 0x01000000;
	osc_frq[1] = 0x00400000;
	init_noise();
	
	// setup basic stuff - clocks, serial, etc
	SystemInit();

	// start serial @ default 115200bps
	Delay_Ms( 100 );
	printf("\r\r\n\nCheep3a\n\r");
	printf("Version: %s\n\r", fwVersionStr);
	printf("Build Date: %s\n\r", bdate);
	printf("Build Time: %s\n\r", btime);

	// init systick @ 1ms rate
	printf("initializing systick...");
	systick_init();
	printf("done.\n\r");
	
	// init LED
	printf("initializing LED...");
	init_led();
	printf("done.\n\r");
	
	// init sync
	printf("initializing sync...");
	sync_init();
	printf("done.\n\r");
	
	// init SPI DAC
	printf("initializing spi dac...");
	spidac_init();
	printf("done.\n\r");
		
	// init ADC
	printf("initializing adc...");
	adc_init();
	printf("done.\n\r");
	
	/* load the operation mode */
	mode = eeprom_read();
	if(mode>7)
	{
		mode = 1;
		eepend = 1;
		eegoal = SysTick_goal(5000);
		printf("Default mode used\n");
	}
	set_led(mode);
	printf("initialized mode to %d\n\r", mode);
	
	printf("looping...\n\r");
	while(1)
	{
		/* check for button press */
		if(SysTick_get_button())
		{
			mode++;
			mode = mode > 7 ? 1 : mode;
			printf("Mode %d\n\r", mode);
			eepend = 1;
			eegoal = SysTick_goal(5000);
			set_led(mode);
		}
		
		/* check if it's time to save the mode */
		if(eepend && !SysTick_check(eegoal))
		{
			eeprom_write(mode);
			eepend = 0;
			printf("Saved mode %d\n\r", mode);
		}
	}
}
