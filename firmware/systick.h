/*
 * systick.h - single-file header for Systick IRQ
 * 07-21-23 E. Brombaugh
 */

#ifndef __systick__
#define __systick__

#include "debounce.h"

/* some bit definitions for systick regs */
#define SYSTICK_SR_CNTIF (1<<0)
#define SYSTICK_CTLR_STE (1<<0)
#define SYSTICK_CTLR_STIE (1<<1)
#define SYSTICK_CTLR_STCLK (1<<2)
#define SYSTICK_CTLR_STRE (1<<3)
#define SYSTICK_CTLR_SWIE (1<<31)

volatile uint32_t systick_cnt;
debounce_state dbs;

/*
 * Start up the SysTick IRQ and init button debounce
 */
void systick_init(void)
{
	uint32_t temp;
	
	/* setup button input on PC7 with pullup */
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
	temp = GPIOC->CFGLR & ~(0xf<<(4*7));
	temp |= (GPIO_CNF_IN_PUPD)<<(4*7);
	GPIOC->CFGLR = temp;
	GPIOC->BSHR = (1<<7);	// pull up
	
	/* init the debouncer */
	init_debounce(&dbs, 10);

	/* disable default SysTick behavior */
	SysTick->CTLR = 0;
	
	/* enable the SysTick IRQ */
	NVIC_EnableIRQ(SysTicK_IRQn);
	
	/* Set the tick interval to 1ms for normal op */
	SysTick->CMP = (FUNCONF_SYSTEM_CORE_CLOCK/1000)-1;
	
	/* Start at zero */
	SysTick->CNT = 0;
	systick_cnt = 0;
	
	/* Enable SysTick counter, IRQ, HCLK/1 */
	SysTick->CTLR = SYSTICK_CTLR_STE | SYSTICK_CTLR_STIE |
					SYSTICK_CTLR_STCLK;
}

/*
 * restart the SysTick IRQ after missed interrupt
 */
void SysTick_Restart(void)
{
	/* push the IRQ out ahead of now */
	SysTick->CMP = SysTick->CNT + (FUNCONF_SYSTEM_CORE_CLOCK/1000)-1;
	
	/* clear IRQ */
	SysTick->SR = 0;
}

/*
 * SysTick ISR counts ticks & updates button state
 * note - the __attribute__((interrupt)) syntax is crucial!
 */
void SysTick_Handler(void) __attribute__((interrupt));
void SysTick_Handler(void)
{
	// move the compare further ahead in time.
	// as a warning, if more than this length of time
	// passes before triggering, you may miss your
	// interrupt.
	SysTick->CMP += (FUNCONF_SYSTEM_CORE_CLOCK/1000);

	/* clear IRQ */
	SysTick->SR = 0;

	/* update counter */
	systick_cnt++;
	
	/* update button debounce */
	debounce(&dbs, (~GPIOC->INDR >> 7)&1);

}

/*
 * get button state
 */
uint8_t SysTick_get_button(void)
{
	uint8_t result = dbs.re;
	dbs.re = 0;
	return result;
}

/*
 * compute goal for Systick counter based on desired delay in ticks
 */
uint32_t SysTick_goal(uint32_t ticks)
{
	return ticks + systick_cnt;
}

/*
 * return FALSE if goal is reached
 */
uint32_t SysTick_check(uint32_t goal)
{
    /**************************************************/
    /* DANGER WILL ROBINSON!                          */
    /* the following syntax is CRUCIAL to ensuring    */
    /* that this test doesn't have a wrap bug         */
    /**************************************************/
	return (((int32_t)systick_cnt - (int32_t)goal) < 0);
}

#endif