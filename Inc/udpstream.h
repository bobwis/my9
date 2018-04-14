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

#define MY_UID 0x12346		// 18 bits  "serial number" of the lightning detector

#ifdef TESTING
#define SERVER_DESTINATION "lightning.local"
#else
#define SERVER_DESTINATION "lightning.vk4ya.space"
#endif


#define UDP_PORT_NO 5000
extern TaskHandle_t xTaskToNotify;

#endif /* UDPSTREAM_H_ */
