/*
 * eeprom.h
 *
 *  Created on: 19Jun.,2018
 *      Author: bob
 */

#ifndef EEPROM_H_
#define EEPROM_H_

/*
RAM (xrw)      : ORIGIN = 0x20000000, LENGTH = 512K
FLASH (rx)     : ORIGIN = 0x8010000, LENGTH = 1984 // 2048K
EEPROM (rx)    : ORIGIN = 0x8008000, LENGTH = 256K
*/
#define EEPROM_START_ADDRESS 0x8008000
#define PAGE_SIZE 32768

#endif /* EEPROM_H_ */
