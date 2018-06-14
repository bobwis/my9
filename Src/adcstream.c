/*
 * adcstream.c
 *
 *  Created on: 22Dec.,2017
 *      Author: bob
 */

#include <stdlib.h>
#include <math.h>
#include "stm32f7xx_hal.h"
#include "adcstream.h"
#include "mydebug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "neo7m.h"

HAL_StatusTypeDef adcstat;

adcbuffer *adcbuf1;
adcbuffer *adcbuf2;
adcbuffer *pktbuf;

uint32_t t2cap[1];  // dma writes t2 capture value on 1pps edge

unsigned int dmabufno = 0;	// the last filled buffer 0 or 1

unsigned int sigprev = 0;	// number of streams let after adc thresh exceeded
volatile unsigned int sigsend = 0;	// flag to tell udp to send sample packet
uint32_t globaladcavg = 0;		// adc average over milli-secs
uint32_t globaladcnoise = 0;	// adc noise peaks average over milli-secs
uint8_t adcbatchid = 0;	// adc sequence number of a batch of 1..n consecutive triggered buffers
uint8_t sendendstatus = 0; 	// flag to send end of capture status
int jabber = 0;			// timeout for spamming trigger

/* Stores the handle of the task that will be notified when the
 transmission is complete. */
volatile TaskHandle_t xTaskToNotify = NULL;

// the two vars below should be moved to more appropriate file
uint8_t netup = 0;				// state of LAN up / down
uint8_t rtseconds = 0;			// real time seconds

/**
 * @brief  DMA transfer complete callback.
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *                the configuration information for the specified DMA module.
 * @retval None
 */
void ADC_MultiModeDMAConvCplt(DMA_HandleTypeDef *hdma) {
	/* Retrieve ADC handle corresponding to current DMA handle */
	ADC_HandleTypeDef* hadc =
			(ADC_HandleTypeDef*) ((DMA_HandleTypeDef*) hdma)->Parent;
//DMA2->HIFCR |= (uint32_t)0x0000001F;
	/* Update state machine on conversion status if not in error state */
	if (HAL_IS_BIT_CLR(hadc->State,
			HAL_ADC_STATE_ERROR_INTERNAL | HAL_ADC_STATE_ERROR_DMA)) {
		/* Update ADC state machine */
		SET_BIT(hadc->State, HAL_ADC_STATE_REG_EOC);

		/* Determine whether any further conversion upcoming on group regular   */
		/* by external trigger, continuous mode or scan sequence on going.      */
		/* Note: On STM32F7, there is no independent flag of end of sequence.   */
		/*       The test of scan sequence on going is done either with scan    */
		/*       sequence disabled or with end of conversion flag set to        */
		/*       of end of sequence.                                            */

		if (ADC_IS_SOFTWARE_START_REGULAR(hadc)
				&& (hadc->Init.ContinuousConvMode == DISABLE)
				&& (HAL_IS_BIT_CLR(hadc->Instance->SQR1, ADC_SQR1_L)
						|| HAL_IS_BIT_CLR(hadc->Instance->CR2, ADC_CR2_EOCS))) {
			/* Disable ADC end of single conversion interrupt on group regular */
			/* Note: Overrun interrupt was enabled with EOC interrupt in          */
			/* HAL_ADC_Start_IT(), but is not disabled here because can be used   */
			/* by overrun IRQ process below.                                      */
			__HAL_ADC_DISABLE_IT(hadc, ADC_IT_EOC);

			/* Set ADC state */
			CLEAR_BIT(hadc->State, HAL_ADC_STATE_REG_BUSY);

			if (HAL_IS_BIT_CLR(hadc->State, HAL_ADC_STATE_INJ_BUSY)) {
				SET_BIT(hadc->State, HAL_ADC_STATE_READY);
			}
		}

		/* Conversion complete callback */
		HAL_ADC_ConvCpltCallback(hadc);
	} else {
		/* Call DMA error callback */
		hadc->DMA_Handle->XferErrorCallback(hdma);
	}
}

/**
 * @brief  DMA half transfer complete callback.
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *                the configuration information for the specified DMA module.
 * @retval None
 */
void ADC_MultiModeDMAHalfConvCplt(DMA_HandleTypeDef *hdma) {
	ADC_HandleTypeDef* hadc =
			(ADC_HandleTypeDef*) ((DMA_HandleTypeDef*) hdma)->Parent;
	/* Conversion complete callback */

//HAL_ADCEx_MultiModeStop_DMA(hadc);		// freeze
//HAL_ADC_Stop(&hadc1);
//HAL_DMA_Abort(&hadc1);
//DMA2->HIFCR |= (uint32_t)0x0000001F;
//	myhalfcomplete++;
	HAL_ADC_ConvHalfCpltCallback(hadc);
}

/**
 * @brief  DMA error callback
 * @param  hdma: pointer to a DMA_HandleTypeDef structure that contains
 *                the configuration information for the specified DMA module.
 * @retval None
 */
void ADC_MultiModeDMAError(DMA_HandleTypeDef *hdma) {
	ADC_HandleTypeDef* hadc =
			(ADC_HandleTypeDef*) ((DMA_HandleTypeDef*) hdma)->Parent;
	hadc->State = HAL_ADC_STATE_ERROR_DMA;
	/* Set ADC error code to DMA error */
	hadc->ErrorCode |= HAL_ADC_ERROR_DMA;

	printf("Multi-mode DMA Error\n");
	HAL_ADC_ErrorCallback(hadc);
}

/**
 * @brief  Enables ADC DMA request after last transfer (Multi-ADC mode) and enables ADC peripheral
 *
 * @note   Caution: This function must be used only with the ADC master.
 *
 * @param  hadc: pointer to a ADC_HandleTypeDef structure that contains
 *         the configuration information for the specified ADC.
 * @param  pData:   Pointer to buffer in which transferred from ADC peripheral to memory will be stored.
 * @param  Length:  The length of data to be transferred from ADC peripheral to memory.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_ADCEx_MultiModeStart_DBDMA(ADC_HandleTypeDef* hadc,
		uint32_t* pData, uint32_t* pData2, uint32_t Length) {
	__IO uint32_t counter = 0;

	/* Check the parameters */
	assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ContinuousConvMode));
	assert_param(IS_ADC_EXT_TRIG_EDGE(hadc->Init.ExternalTrigConvEdge));
	assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DMAContinuousRequests));

	/* Process locked */
	__HAL_LOCK(hadc);

	/* Check if ADC peripheral is disabled in order to enable it and wait during
	 Tstab time the ADC's stabilization */
	if ((hadc->Instance->CR2 & ADC_CR2_ADON) != ADC_CR2_ADON) {
		/* Enable the Peripheral */
		__HAL_ADC_ENABLE(hadc);

		/* Delay for temperature sensor stabilization time */
		/* Compute number of CPU cycles to wait for */
		counter = (ADC_STAB_DELAY_US * (SystemCoreClock / 1000000));
		while (counter != 0) {
			counter--;
		}
	}

	/* Start conversion if ADC is effectively enabled */
	if (HAL_IS_BIT_SET(hadc->Instance->CR2, ADC_CR2_ADON)) {
		/* Set ADC state                                                          */
		/* - Clear state bitfield related to regular group conversion results     */
		/* - Set state bitfield related to regular group operation                */
		ADC_STATE_CLR_SET(hadc->State,
				HAL_ADC_STATE_READY | HAL_ADC_STATE_REG_EOC | HAL_ADC_STATE_REG_OVR,
				HAL_ADC_STATE_REG_BUSY);

		/* If conversions on group regular are also triggering group injected,    */
		/* update ADC state.                                                      */
		if (READ_BIT(hadc->Instance->CR1, ADC_CR1_JAUTO) != RESET) {
			ADC_STATE_CLR_SET(hadc->State, HAL_ADC_STATE_INJ_EOC, HAL_ADC_STATE_INJ_BUSY);
		}

		/* State machine update: Check if an injected conversion is ongoing */
		if (HAL_IS_BIT_SET(hadc->State, HAL_ADC_STATE_INJ_BUSY)) {
			/* Reset ADC error code fields related to conversions on group regular */
			CLEAR_BIT(hadc->ErrorCode, (HAL_ADC_ERROR_OVR | HAL_ADC_ERROR_DMA));
		} else {
			/* Reset ADC all error code fields */
			ADC_CLEAR_ERRORCODE(hadc);
		}

		/* Process unlocked */
		/* Unlock before starting ADC conversions: in case of potential           */
		/* interruption, to let the process to ADC IRQ Handler.                   */
		__HAL_UNLOCK(hadc);

		/* Set the DMA transfer complete callback */
		hadc->DMA_Handle->XferCpltCallback = ADC_MultiModeDMAConvM0Cplt;
		hadc->DMA_Handle->XferM1CpltCallback = ADC_MultiModeDMAConvM1Cplt;

		/* Set the DMA half transfer complete callback */
		hadc->DMA_Handle->XferM1HalfCpltCallback = NULL;
		hadc->DMA_Handle->XferHalfCpltCallback = NULL;

		/* Set the DMA error callback */
		hadc->DMA_Handle->XferErrorCallback = ADC_MultiModeDMAError;

		/* Manage ADC and DMA start: ADC overrun interruption, DMA start, ADC     */
		/* start (in case of SW start):                                           */

		/* Clear regular group conversion flag and overrun flag */
		/* (To ensure of no unknown state from potential previous ADC operations) */
		__HAL_ADC_CLEAR_FLAG(hadc, ADC_FLAG_EOC);

		/* Enable ADC overrun interrupt */
		__HAL_ADC_ENABLE_IT(hadc, ADC_IT_OVR);

		if (hadc->Init.DMAContinuousRequests != DISABLE) {
			/* Enable the selected ADC DMA request after last transfer */
			ADC->CCR |= ADC_CCR_DDS;
		} else {
			/* Disable the selected ADC EOC rising on each regular channel conversion */
			ADC->CCR &= ~ADC_CCR_DDS;
		}

		/* Enable the DMA Stream */
		//HAL_DMA_Start_IT(hadc->DMA_Handle, (uint32_t)&ADC->CDR, (uint32_t)pData, Length);
		HAL_DMAEx_MultiBufferStart_IT(hadc->DMA_Handle, (uint32_t) &ADC->CDR,
				(uint32_t) pData, (uint32_t) pData2, Length);
		/* if no external trigger present enable software conversion of regular channels */
		if ((hadc->Instance->CR2 & ADC_CR2_EXTEN) == RESET) {
			/* Enable the selected ADC software conversion for regular group */
			hadc->Instance->CR2 |= (uint32_t) ADC_CR2_SWSTART;
		}
	}

	/* Return function status */
	return HAL_OK;
}

// this does some quick stats on the adc values and overloads them in the packet time fields
// to help evauate debug adc noise
#if 0
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)	// adc conversion done
{
	register uint32_t hi = 0, lo = 0xffffffff, avg = 0;
	int i;
	adcbuffer *buf;

	if ((myfullcomplete & 1) == 0) {
		buf = pktbuf;
	} else {
		buf = &((*pktbuf)[UDPBUFSIZE / 4]);
	}

	for (i = 8; i < (UDPBUFSIZE / 2); i++) {		// uint16
		hi = (hi > ((uint16_t *) *buf)[i]) ? hi : ((uint16_t *) *buf)[i];
		lo = (lo < ((uint16_t *) *buf)[i]) ? lo : ((uint16_t *) *buf)[i];
		avg = avg + ((uint16_t *) *buf)[i];
	}

	(*buf)[0] = ((uint16_t *) *buf)[4];	// first two adc readings
	(*buf)[1] = hi;
	(*buf)[2] = lo;
	(*buf)[3] = avg / ((UDPBUFSIZE / 2) - 8);

	myfullcomplete++;
//	HAL_ADCEx_MultiModeStop_DMA(hadc);		// freeze
//	HAL_ADC_Stop(&hadc1);
//	HAL_DMA_Abort(&hadc1);
}
#endif

#if 0
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)// adc conversion done (DMA half complete)
{
	volatile uint32_t sr;

	HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_1);

	sr = DMA2_Stream4->CR & (1<<19);
//	while (DMA2_Stream4->CR & (1<<19) == sr)		// still doing buffer
//		;		// wait
//	while ((DMA2_Stream4->CR & (1<<19)) == 1)		// still doing buffer
//	while ((DMA2_Stream4->CR & (1<<19)) == 0) // (1<<19))		// still doing buffer

	myhalfcomplete = 1;// second buffer has completed

//	HAL_ADC_ConvCpltCallback(hadc); 	// then process
}
#endif

#define WINSIZE 256
#define WINSHIFT 8
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)// adc conversion done (DMA complete)
{
	register uint32_t timestamp;
	register int32_t i;
	register uint8_t j;
	adcbuffer *buf;
	adc16buffer *adcbuf16;
	static uint32_t adcbgbaseacc = 0;		// avg adc level per buffer
	static uint32_t samplecnt = 0;
	static uint32_t ledhang = 0;
	static uint8_t adcbufnum = 0;		// adc sequence number
	int32_t thiswindiff = 0;
	uint16_t thissamp = 0;
	static int32_t windiff[WINSIZE] = { 0 };// past window differences from the window mean
	static uint16_t lastsamp[WINSIZE] = { 0 };// last sample saved to calc global mean
	static int16_t winmean = 0;	// sliding window mean
	static int16_t meanwindiff = 0;	// sliding mean of window differences
	static int32_t wdacc = 0;	// window difference accumulator
	static int32_t wmeanacc = 0;	// window mean accumulator
	volatile static uint16_t lastmeanwindiff = 0;

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	timestamp = TIM2->CNT;			// real time

	if (dmabufno == 1) {		// second buffer is ready
		buf = &((*pktbuf)[(UDPBUFSIZE / 4)]);
	} else {
		buf = pktbuf;
	}

	adcbuf16 = &((uint16_t *) *buf)[8];

//	(*buf)[0] = UDP seq and packet flags	// set in udpstream.c
	(*buf)[1] = (statuspkt.uid << 16) | (adcbatchid << 8) | (rtseconds << 2)
			| (adcbufnum++ & 3);	// ADC completed packet counter (24 bits)
	(*buf)[2] = statuspkt.epochsecs; // statuspkt.NavPvt.iTOW;
	(*buf)[3] = timestamp;

	if (sigsend) {		// oops overrun
		statuspkt.adcudpover++;		// debug adc overrun udp
		sigsend = 0;		// cancel it anyway
	}

	for (i = 0; i < (ADCBUFSIZE / 2); i++) {		// scan the buffer content
		j = i & (WINSIZE - 1);
		thissamp = (*adcbuf16)[i];

		adcbgbaseacc += thissamp; // accumulator used to find avg level of signal over long time (for base)

		wmeanacc = wmeanacc + thissamp - lastsamp[j];		// window mean acc
		winmean = wmeanacc >> (WINSHIFT);// divide to find the new window mean
		lastsamp[j] = thissamp;			// save last samples

		thiswindiff = abs(thissamp - winmean);// find difference from window mean
		wdacc = wdacc - windiff[j] + thiswindiff; // difference accumulator for WINSIZE samples

		lastmeanwindiff = meanwindiff;

		meanwindiff = wdacc >> (WINSHIFT); // sliding mean of window differences
		windiff[j] = meanwindiff;	// store latest window mean of differences

		if (abs(meanwindiff) > (abs(lastmeanwindiff) + 1)) { // if new mean diff > last mean diff +1
			sigsend = 1;	// trigger greater than running mean
		}
	} // end for i

	if (sigsend) {
		HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_SET);	// blue led
		if (sigprev == 0)		// no trigger last time, so this is a new event
			adcbatchid++;		// start a new adc batch number
		sigprev = 1;	// remember this trigger for next packet
		ledhang = 1000;
		statuspkt.trigcount++;	//  no of triggered packets detected

	} else {			// no trigger
		if (sigprev) {		// but there was a trigger the last packet
			sendendstatus = 1;	// so tell udpstream to send the end of sequence status packet
		}
		sigprev = 0;
		HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_RESET);	// blue led
	}

	if (ledhang) {		// this could be moved to the low priority process
		ledhang--;
		HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_SET);		// red led on
	} else {
		HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_RESET);	 // red led off
	}

	globaladcnoise = meanwindiff;
	if (globaladcnoise == 0)
		globaladcnoise = statuspkt.adcbase;		// dont allow zero peaks

	statuspkt.adctrigoff = TRIG_THRES
			+ (abs(globaladcnoise - statuspkt.adcbase));

	if (statuspkt.adctrigoff > 4095)
		statuspkt.adctrigoff = 4095;

	samplecnt++;

	if (samplecnt == 2048) {		// 2k adc bufffers sampled approx 0.5 sec
		globaladcavg = adcbgbaseacc / (ADCBUFSIZE / 2) >> 11;
		adcbgbaseacc = 0;
		samplecnt = 0;
	}

	statuspkt.adcnoise = (globaladcnoise & 0xfff);	// agc
	statuspkt.adcbase = (globaladcavg & 0xfff);	// agc

	if (xTaskToNotify == NULL) {
		printf("Notify task null\n");
	} else if (sigsend) {
		vTaskNotifyGiveFromISR(xTaskToNotify, &xHigherPriorityTaskWoken);
		// signal the detection processing of this packet to the back-end
		/* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
		 should be performed to ensure the interrupt returns directly to the highest
		 priority task.  The macro used for this purpose is dependent on the port in
		 use and may be called portEND_SWITCHING_ISR(). */
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}

void ADC_MultiModeDMAConvM0Cplt(ADC_HandleTypeDef* hadc) {
	dmabufno = 0;
	HAL_ADC_ConvCpltCallback(hadc);
}

void ADC_MultiModeDMAConvM1Cplt(ADC_HandleTypeDef* hadc) {
	dmabufno = 1;
	HAL_ADC_ConvCpltCallback(hadc);
}

void startadc() {
	int i, lastbuf = 0;
//	uint16_t *adcbufdum1, *adcbufdum2;		// debug
//	adcbufdum1 = pvPortMalloc(UDPBUFSIZE);	//  dummy buffer
//	adcbufdum2 = pvPortMalloc(UDPBUFSIZE);	//  dummy buffer

	statuspkt.clktrim = 107000000;
	statuspkt.adcpktssent = 0;

	printf("Startadc:\n");
	osDelay(1000);
// get some heap for the ADC stream DMA buffer 1
	pktbuf = pvPortMalloc(UDPBUFSIZE * 2);	// two buffers concatenated
	if (pktbuf == NULL) {
		printf("pvPortMalloc returned nil for pktbuf\n");
		for (;;)
			;
	}
	if (((uint32_t) pktbuf & 3) > 0) {
		printf("******** pvPortMalloc not on word boundary *********\n");
	}

//	printf("(&(*pktbuf)[0])=0x%x ", &((*pktbuf)[0]));
//	printf("(&(*pktbuf)[UDPBUFSIZE / 4])=0x%x\n", &((*pktbuf)[UDPBUFSIZE / 4]));

	for (i = 0; i < UDPBUFSIZE / 4; i++) {	// fill buffers, 4 bytes at a time
		(*pktbuf)[i] = 0x55555555;
	}
	for (i = UDPBUFSIZE / 4; i < UDPBUFSIZE / 2; i++) {	// fill buffers, 4 bytes at a time
		(*pktbuf)[i] = 0xaaaaaaaa;
	}

	adcbuf1 = &(*pktbuf)[ADCBUFHEAD / 4];// leave room in start of first buffer
	adcbuf2 =
			&(*pktbuf)[(ADCBUFHEAD / 4) + (ADCBUFSIZE / 4) + (ADCBUFHEAD / 4)];	// leave room in start of 2nd buffer

	adcstat = HAL_ADCEx_MultiModeStart_DBDMA(&hadc1, adcbuf1, adcbuf2,
			(ADCBUFSIZE / 2));		// len in 16bit words

//	adcstat = HAL_ADCEx_MultiModeStart_DBDMA(&hadc1, adcbufdum1, adcbufdum2, (ADCBUFSIZE / 4));		// DEBUG
	printf("ADC_MM_Start returned %u\r\n", adcstat);

	if (HAL_ADC_Start(&hadc3) != HAL_OK)
		printf("ADC3 failed start\r\n");
	if (HAL_ADC_Start(&hadc2) != HAL_OK)
		printf("ADC2 failed start\r\n");
	if (HAL_ADC_Start(&hadc1) != HAL_OK)
		printf("ADC1 failed start\r\n");
#if 0
	while (1) {
//			HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_RESET);	 // red led off
		HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_SET);// blue led on
		while (lastbuf == myfullcomplete)// wait for buf1 to fill
		vTaskDelay(0);// wait
//timer			((uint16_t *)*pktbuf)[UDPBUFSIZE/2] = i; // 0xaaaa5555;
		lastbuf = myfullcomplete;
		HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_RESET);// blue led off
//			HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_SET);		// red led on

		//	myhexDump ("INITBUFF1---------------------------------------", *adcbuf1, ADCBUFLEN*2);
	}
#endif
}
