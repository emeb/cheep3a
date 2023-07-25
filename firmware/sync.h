/*
 * sync.h - single-file header for sync input
 */

#ifndef __sync__
#define __sync__

uint8_t psync;

#define get_sync() ((~GPIOD->INDR >> 6)&1)

/*
 * init the sync input
 */
void sync_init(void)
{
	/* setup sync input on PD6 with pullup */
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
	uint32_t temp = GPIOD->CFGLR & ~(0xf<<(4*6));
	temp |= (GPIO_CNF_IN_FLOATING)<<(4*6);
	GPIOD->CFGLR = temp;
	
	psync = get_sync();
}

/*
 * test for sync rising edge
 */
uint8_t sync_re(void)
{
	uint8_t result = 0;
	
	uint8_t new_sync = get_sync();
	
	if((psync == 0) && (new_sync != 0))
		result = 1;
	
	psync = new_sync;
	
	return result;
}

#endif
