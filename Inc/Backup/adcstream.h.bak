/*
 * adcstream.h
 *
 *  Created on: 22Dec.,2017
 *      Author: bob
 */

#ifndef ADCSTREAM_H_
#define ADCSTREAM_H_

#define UDPBUFSIZE (1472)
#define ADCBUFHEAD 16
#define ADCBUFSIZE (UDPBUFSIZE-ADCBUFHEAD)

#define DETECT_THRES 1000		// adc detection initial trig pkt thresh (dynamic now)
#define TRIG_THRES 110			// adc trigger level above avg noise
#define HANGPRESET 1			// packets sampling continues after a trigger event

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

void startadc(void);

extern unsigned int myfullcomplete;
extern unsigned int myhalfcomplete;

extern unsigned int hangcount;		// number of streams let after adc thresh exceeded
extern uint16_t adcthreshold;		// adc attention level threshold
extern uint32_t globaladcavg;		// adc global average level over 100-200msec
extern uint32_t t2avg;				// cpu clock trim variable

#endif /* ADCSTREAM_H_ */
