/*
 * version.h
 *
 *  Created on: 20Mar.,2018
 *      Author: bob
 */

#ifndef VERSION_H_
#define VERSION_H_

#define MAJORVERSION 0
#define MINORVERSION 4

// initial serial number of the lightning detector 16 bits
//#define MY_UID 1051/1001 // Bobs home development version
//#define MY_UID 1002 // Ormiston home production version
//#define MY_UID 1003	// Mount Cotton production version
//#define MY_UID 1004 // Bobs home production version
#define MY_UID 1999 // Change this or add your own UID here



// TESTING Speeds up the frequency of status packets
#if		0
#define TESTING
#endif

#endif /* VERSION_H_ */
