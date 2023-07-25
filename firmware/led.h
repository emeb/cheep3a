/*
 * led.h - single-file include RGB LED Driver for Cheep3a
 * 07-21-23 E. Brombaugh
 */

#ifndef __led__
#define __led__

/*
 * Red = PD0
 * Green = PC0
 * Blue = PC3
 */
enum led_values {
	LED_RED = 1,
	LED_GREEN = 2,
	LED_BLUE = 4,
};

/*
 * initialize LED GPIO
 */
void init_led(void)
{
	uint32_t temp;
	
	// GPIO C & D
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;
	
	// C0 and C3
	temp = GPIOC->CFGLR & ~((0xf<<(4*0)) || (0xf<<(4*3)));
	temp |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0) |
		(GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*3);
	GPIOC->CFGLR = temp;
	
	// D0
	temp = GPIOD->CFGLR & ~(0xf<<(4*0));
	temp |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP)<<(4*0);
	GPIOD->CFGLR = temp;
}

/*
 * set LED on/off
 */
void set_led(uint8_t led)
{
	if(led & LED_RED)
		GPIOD->BSHR = (1<<16);
	else
		GPIOD->BSHR = 1;

	if(led & LED_GREEN)
		GPIOC->BSHR = (1<<16);
	else
		GPIOC->BSHR = 1;

	if(led & LED_BLUE)
		GPIOC->BSHR = (8<<16);
	else
		GPIOC->BSHR = 8;
}

#endif
