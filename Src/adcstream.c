/*
 * adcstream.c
 *
 *  Created on: 22Dec.,2017
 *      Author: bob
 */

#include "stm32f7xx_hal.h"
#include "adcstream.h"
#include "mydebug.h"
#include "freertos.h"
#include "neo7m.h"

HAL_StatusTypeDef adcstat;

adcbuffer *adcbuf1;
adcbuffer *adcbuf2;
adcbuffer *pktbuf;

uint32_t t2cap[1];
uint32_t t2avg;		// cpu clock trim

uint16_t adcthreshold = DETECT_THRES;

unsigned int myfullcomplete = 0;
unsigned int myhalfcomplete = 0;

unsigned int hangcount = 0;	// number of streams let after adc thresh exceeded
uint32_t globaladcavg = 0;		// adc average over 100-200msec

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
//  HAL_ADCEx_MultiModeStop_DMA(hadc);		// freeze
//HAL_ADC_Stop(&hadc1);
// HAL_DMA_Abort(&hadc1);
	myhalfcomplete++;
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
		hadc->DMA_Handle->XferCpltCallback = ADC_MultiModeDMAConvCplt;

		/* Set the DMA half transfer complete callback */
//		hadc->DMA_Handle->XferHalfCpltCallback = ADC_MultiModeDMAHalfConvCplt;
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
//HAL_ADC_Stop(&hadc1);
//HAL_DMA_Abort(&hadc1);
}
#endif

#if 1
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)	// adc conversion done
{
	register uint32_t timestamp, i;
	adcbuffer *buf;
	adc16buffer *adcbuf16;
	uint32_t adcbgnoise = 0;		// avg adc level per buffer
	static uint32_t samplecnt = 0, avg = 0;
	static uint32_t ledhang = 0;

	timestamp = TIM2->CNT;			// real time

	if ((myfullcomplete & 1) == 0) {	// select the correct buffer of the two
		buf = pktbuf;
	} else {
		buf = &((*pktbuf)[UDPBUFSIZE / 4]);
	}
	adcbuf16 = &((uint16_t *) *buf)[8];

//	(*buf)[0] = UDP seq and packet flags	// set in udpstream.c
	(*buf)[1] = (myfullcomplete & 0xff) | ((statuspkt.uid & 0x3ffff) << 8) | (statuspkt.NavPvt.sec << 26);// ADC completed packet counter (24 bits)
//	(*buf)[2] = 1pps capture cnt;		// 1pps counter capture  (set by DMA)
	(*buf)[3] = timestamp;

	for (i = 0; i < (ADCBUFSIZE / 2); i++) {
		if ((*adcbuf16)[i] > adcthreshold) {
			hangcount = HANGPRESET + 1;		// enable detection processing
			ledhang = 1000;
		}
		adcbgnoise += (*adcbuf16)[i];
	}
	if (hangcount) {
		hangcount--;

	}
	if (ledhang) {
		ledhang--;
		HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_SET);	 // red led on
	} else {
		HAL_GPIO_WritePin(GPIOB, LD3_Pin, GPIO_PIN_RESET);	 // red led off
	}
//	HAL_GPIO_TogglePin(GPIOB, LD3_Pin);		// Red LED
	myfullcomplete++;
	samplecnt++;

	avg += adcbgnoise / (ADCBUFSIZE / 2);
	if (samplecnt == 256) {			// 256 adc bufffers sampled
		globaladcavg = avg >> 8;		// maybe 100-200mSec
		adcbgnoise = 0;
		avg = 0;
		samplecnt = 0;
	}
	adcthreshold = (globaladcavg & 0xfff) + TRIG_THRES;	// agc
}
#endif

void startadc() {
	int i, lastbuf = 0;
	uint16_t *adcbufdum1, *adcbufdum2;		// debug
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
	if ((uint32_t) pktbuf & 3 > 0) {
		printf("******** pvPortMalloc not on word boundary *********\n");
	}

	printf("(&(*pktbuf)[0])=0x%x ", &((*pktbuf)[0]));
	printf("(&(*pktbuf)[UDPBUFSIZE / 4])=0x%x\n", &((*pktbuf)[UDPBUFSIZE / 4]));

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
			(ADCBUFSIZE / 4));		// len in 16bit words

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
