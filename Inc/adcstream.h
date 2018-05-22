/*
 * adcstream.h
 *
 *  Created on: 22Dec.,2017
 *      Author: bob
 */


#ifndef ADCSTREAM_H_
#define ADCSTREAM_H_

#include "freertos.h"
#include "task.h"

#define UDPBUFSIZE (1472)
#define ADCBUFHEAD 16
#define ADCBUFSIZE (UDPBUFSIZE-ADCBUFHEAD)

#define TRIG_THRES 120			// adc trigger level above avg noise (should be calculated, not fixed)

typedef uint32_t adcbuffer[ADCBUFSIZE/2];
typedef uint16_t adc16buffer[ADCBUFSIZE];
typedef uint8_t adc8buffer[ADCBUFSIZE*2];

extern adcbuffer *adcbuf1;
extern adcbuffer *adcbuf2;
extern adcbuffer *pktbuf;

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;
extern DMA_HandleTypeDef hdma_adc1;

void ADC_MultiModeDMAConvCplt(DMA_HandleTypeDef *hdma);
void ADC_MultiModeDMAError(DMA_HandleTypeDef *hdma);
void ADC_MultiModeDMAHalfConvCplt(DMA_HandleTypeDef *hdma);

void ADC_MultiModeDMAConvM0Cplt(ADC_HandleTypeDef* hadc);
void ADC_MultiModeDMAConvM1Cplt(ADC_HandleTypeDef* hadc);

void startadc(void);

extern unsigned int dmabufno;

extern unsigned int sigprev;		// number of streams let after adc thresh exceeded
volatile extern unsigned int sigsend;	// flag to tell udp to send sample packet

extern uint32_t globaladcavg;		// adc global average level over 100-200msec
extern uint32_t t2avg;				// cpu clock trim variable

extern uint8_t netup;				// state of LAN up / down
extern uint16_t padding1;			// unused
extern uint8_t rtseconds;			// real time seconds, synced to the gps 1pps pulse
extern uint8_t	adcbatchid;	// adc sequence number of a batch of 1..n consecutive triggered buffers
extern int jabber;			// jabber timeout for spamming detections



#endif /* ADCSTREAM_H_ */
