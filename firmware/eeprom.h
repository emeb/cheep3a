/*
 * eeprom.h - single file header for 15-bit eeprom emulation in flash
 * 07-23-23 E. Brombaugh
 */

#ifndef __eeprom__
#define __eeprom__

/* set this to the start of the last flash page */
#define EEPROM_BASE 0x08003FC0

/*
 * write 15 bits to the next available half-word in the reserved flash page
 */
void eeprom_write(uint16_t data)
{
	uint16_t *word = (uint16_t *)(EEPROM_BASE);
	int8_t cnt = 0;
	
	printf("eeprom_write\n\r");
	
	/* find next available half-word in the page */
	while(cnt < 32)
	{
		if((*word & 0x8000) == 0x8000)
			break;
		cnt++;
		word++;
	}
	
	printf("\t cnt = %d, word = 0x%08lX\n\r", cnt, (uint32_t)word);
	
	/* unlock flash */
	if(FLASH->CTLR & 0x80) 
	{
		FLASH->KEYR = 0x45670123;
		FLASH->KEYR = 0xCDEF89AB;
	}
	
	/* if page is full then erase it and start over */
	if(cnt == 32)
	{
		printf("\t Erasing...\n\r");
		
		/* reset word pointer to start of page */
		word = (uint16_t *)(EEPROM_BASE);

		/* unlock fast mode */
		if(FLASH->CTLR & 0x8000) 
		{
			FLASH->MODEKEYR = 0x45670123;
			FLASH->MODEKEYR = 0xCDEF89AB;
		}
			
		/* wait for not busy */
		while( FLASH->STATR & FLASH_STATR_BSY );
		
		/* set PAGE_ER, address, start fast erase and wait to finish */
		FLASH->CTLR = CR_PAGE_ER;
		FLASH->ADDR = (intptr_t)word;
		FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
		while( FLASH->STATR & FLASH_STATR_BSY );
		
		printf("\t FLASH->STATR = 0x%08lX\n\r", FLASH->STATR);
		
		/* disable fast mode */
		FLASH->CTLR = 0x8000;
	}
	
	/* "normal" half-word write to flash */
	printf("\t Writing...\n\r");
	FLASH->CTLR = CR_PG_Set;
	*word = data & 0x7FFF;	// clear top bit to flag used location
	while( FLASH->STATR & FLASH_STATR_BSY );

	printf("\t FLASH->STATR = 0x%08lX\n\r", FLASH->STATR);
	
	/* disable programming and relock */
	FLASH->CTLR = 0x80;
	
	/* HW delay during flash ops causes missed systick IRQ so restart */
	SysTick_Restart();
}

/*
 * read the most recently written half-word in the reserved flash page
 */
uint16_t eeprom_read(void)
{
	uint16_t result = 0xFFFF;
	uint16_t *word = (uint16_t *)(EEPROM_BASE + 62);
	int8_t cnt = 0;
	
	printf("eeprom_read\n\r");
	
	/* scan back from the end */
	while(cnt < 32)
	{
		/* did we find a valid word? */
		if(!(*word & 0x8000))
		{
			result = *word;
			break;
		}
		
		/* no - try again */
		cnt++;
		word--;
	}
	printf("\t cnt = %d, result = 0x%04X\n\r", cnt, result);
	
	return result;
}

#endif
