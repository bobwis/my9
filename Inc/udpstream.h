/*
 * udpstream.h
 *
 *  Created on: 22Dec.,2017
 *      Author: bob
 */

#ifndef UDPSTREAM_H_
#define UDPSTREAM_H_

#include "version.h"

void startudp(void);

#ifdef TESTING
//#define SERVER_DESTINATION "lightning.local"
#define SERVER_DESTINATION "10.10.201.243"
#else
//#define SERVER_DESTINATION "lightning.vk4ya.space"
//#define SERVER_DESTINATION "112.213.32.44"
#define SERVER_DESTINATION "10.10.201.159"
#endif

#define UDP_PORT_NO 5000
extern TaskHandle_t xTaskToNotify;

// status packet types
#define TIMED 2
#define ENDSEQ 1

#endif /* UDPSTREAM_H_ */
