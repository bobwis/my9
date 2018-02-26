/*
h * neo7m.h
 *
 *  Created on: 14Jan.,2018
 *      Author: bob
 */

#ifndef NEO7M_H_
#define NEO7M_H_

/*
 * neo7m.h
 *
 * Created: 13/09/2017 5:40:43 PM
 *  Author: bob
 *  Modified from Freqref project 14/01/2018
 */


/**
 * Derived in part from UBX GPS Library
 * Created by Danila Loginov, July 23, 2016
 * https://github.com/1oginov/UBX-GPS-Library
 */


//const unsigned char UBXGPS_HEADER[] = { 0xB5, 0x62 };

struct statpkt {
	uint32_t udpcount;		// udp packet sent index (24 bits, other 8 bits are packet type)

	struct UbxGpsNavPvt {
		//        Type  Name           Unit   Description (scaling)
		unsigned  long  iTOW;       // ms     GPS time of week of the navigation epoch. See the description of iTOW for details.                     0
		unsigned  short year;       // y      Year UTC																			4
		unsigned  char  month;      // month  Month, range 1..12 UTC																6
		unsigned  char  day;        // d      Day of month, range 1..31 UTC														7
		unsigned  char  hour;       // h      Hour of day, range 0..23 UTC														8
		unsigned  char  min;        // min    Minute of hour, range 0..59 UTC									9
		unsigned  char  sec;        // s      Seconds of minute, range 0..60 UTC													10
		char            valid;      // -      Validity Flags (see graphic below)										11
		unsigned  long  tAcc;       // ns     Time accuracy estimate UTC													12
		long            nano;       // ns     Fraction of second, range -1e9..1e9 UTC																16
		unsigned  char  fixType;    // -      GNSSfix Type, range 0..5															20
		char            flags;      // -      Fix Status Flags (see graphic below)												21
		unsigned  char  reserved1;  // -      Reserved							22
		unsigned  char  numSV;      // -      Number of satellites used in Nav Solution					23
		long            lon;        // deg    Longitude (1e-7)									24
		long            lat;        // deg    Latitude (1e-7)												28
		long            height;     // mm     Height above Ellipsoid																32
		long            hMSL;       // mm     Height above mean sea level																		36
		unsigned  long  hAcc;       // mm     Horizontal Accuracy Estimate																			40
		unsigned  long  vAcc;       // mm     Vertical Accuracy Estimate																	44
		long            velN;       // mm/s   NED north velocity														48
		long            velE;       // mm/s   NED east velocity															52
		long            velD;       // mm/s   NED down velocity															56
		long            gSpeed;     // mm/s   Ground Speed (2-D)														60
		long            heading;    // deg    Heading of motion 2-D (1e-5)												64
		unsigned  long  sAcc;       // mm/s   Speed Accuracy Estimate													68
		unsigned  long  headingAcc; // deg    Heading Accuracy Estimate (1e-5)											72
		unsigned  short pDOP;       // -      Position DOP (0.01)														76
		short           reserved2;  // -      Reserved																	78
		unsigned  long  reserved3;  // -      Reserved																	80
		} NavPvt;

	uint32_t clktrim;		// Nominal 108MHz clock is actually this frequency
	uint32_t uid;			// only 18 bits used
	uint32_t adcpktssent;	// Number of ADC pks sent in this trigger event
	uint16_t adctrigoff;	// adc trigger threshold above noise
	uint16_t adcnoise;		// average background noise seen by ADC

} statuspkt; __attribute__((aligned(4),packed)) CHALLENGE;

// set up the newo7 as we want it
extern void setupneo(void);

// Update NavPvt
extern void updategps(void);

#endif /* NEO7M_H_ */


