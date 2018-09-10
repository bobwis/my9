/*
 * eeprom.c
 *
 *  Created on: 19Jun.,2018
 *      Author: bob
 */

// emulate eeprom using STM32 flash memory
// based on http://www.sonic2kworld.com/blog/using-stm32f-flash-as-eeprom-the-easy-way (also derived from elsewhere)
// Included headers
//-----------------------------------
#include "eeprom.h"
#include "stm32f7xx_hal.h"

// Functions
//-----------------------------------

//----------------------------------------------------------------------------------
// Name: ReadEE
// Function: Read data from the EEPROM
// Parameters: address, pointer to byte location where read data is to be stored
// Returns: 0
//----------------------------------------------------------------------------------
int ReadEE(uint32_t address, uint32_t *Data){
	uint32_t Address = EEPROM_START_ADDRESS;								// Base address in FLASH
	uint32_t dataword;														// Temp storage for read from FLASH

	if(address > PAGE_SIZE){
		return -1;															// Address is greater than EEPROM size, return -1
	}

	// Compute the read address into the FLASH memory
	// Passed parameter does not need to know about alignment and such
	Address = Address + (address & 0x3FFFC);									// Compute address from supplied address
	dataword = (*(__IO uint32_t*)(Address));								// Retrieve dword from flash memory (Warning- it can only be dword boundaries)
	*Data = *(((uint32_t *)&dataword) + (address & 0x03));
	return 0;
}

//-----------------------------------------------------------------------------------
// Name: WriteEE
// Function: Write data to the EEPROM
// Parameter: relative EEPROM address, data
// Returns: Result of operation
//-----------------------------------------------------------------------------------
int WriteEE(uint32_t address, uint32_t Data){
	uint32_t Address = EEPROM_START_ADDRESS;
	uint32_t dataword;

	if(address > PAGE_SIZE){
		return -1;
	}

	// Compute destination write address in the FLASH
	Address = Address + (address & 0x3FFFC);

	/**
	  * @brief  Program byte, halfword, word or double word at a specified address
	  * @param  TypeProgram  Indicate the way to program at a specified address.
	  *                           This parameter can be a value of @ref FLASH_Type_Program
	  * @param  Address  specifies the address to be programmed.
	  * @param  Data specifies the data to be programmed
	  *
	  * @retval HAL_Status
	  *
	  * *TypeDef HAL Status
	  *TypeDef  HAL_OK       = 0x00U,
  HAL_ERROR    = 0x01U,
  HAL_BUSY     = 0x02U,
  HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;*

#define FLASH_TYPEPROGRAM_BYTE        ((uint32_t)0x00U)  //!< Program byte (8-bit) at a specified address
#define FLASH_TYPEPROGRAM_HALFWORD    ((uint32_t)0x01U)  //!< Program a half-word (16-bit) at a specified address
#define FLASH_TYPEPROGRAM_WORD        ((uint32_t)0x02U)  //!< Program a word (32-bit) at a specified address
#define FLASH_TYPEPROGRAM_DOUBLEWORD  ((uint32_t)0x03U)  //!< Program a double word (64-bit) at a specified address
 */
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, Data) != HAL_OK) {
		return -1;
	}
	return 0;
}


// test the eeprom emulation
void testeeprom(void) {
	int i;
	uint32_t data[64];

	osDelay(6000);
	printf("Reading EEPROM 1\n");
	for (i=0; i<64; i++) {
		ReadEE(i*4, &(data[i]));
	}
	myhexDump ("", data, 256);

	printf("Writing EEPROM\n");
	HAL_FLASH_Unlock();
	for (i=0; i<64; i++) {
		if ( WriteEE(i*4, (i<<24) | (i<<16) | (i<<8)  | i)) {
			printf("WriteEE Failed at %d\n",i);
		}
	}
	HAL_FLASH_Lock();

	printf("Reading EEPROM 2\n");
	for (i=0; i<64; i++) {
		ReadEE(i*4, &(data[i]));
	}
	myhexDump ("", data, 256);


	while(1)
		;

}
