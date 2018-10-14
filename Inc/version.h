/*
 * version.h
 *
 *  Created on: 20Mar.,2018
 *      Author: bob
 */

#ifndef VERSION_H_
#define VERSION_H_

#define MAJORVERSION 0
#define MINORVERSION 5

// TESTING Speeds up the frequency of status packets
#if		0
#define TESTING
#endif

#ifdef TESTING
#define MY_UID 1071	// 16 bits  "serial number" of the lightning detector
#else
#define MY_UID 1021	// 16 bits  "serial number" of the lightning detector
#endif

#endif /* VERSION_H_ */
