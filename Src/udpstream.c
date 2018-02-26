/*
 * udpstream.c
 *
 *  Created on: 22Dec.,2017
 *      Author: bob
 */
#include "lwip.h"
#include "udpstream.h"
#include "adcstream.h"
#include "mydebug.h"
#include "freertos.h"
#include "neo7m.h"

extern uint32_t t1sec;

void myudp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
		struct ip_addr *addr, u16_t port) {
	if (p != NULL) {
		/* send received packet back to sender */
		udp_sendto(pcb, p, addr, port);
		/* free the pbuf */
		pbuf_free(p);
	}
}

void startudp() {
	struct udp_pcb *pcb;
	volatile struct pbuf *p, *p1, *p2, *ps;
	struct ip4_addr destip;
	uint32_t lastsent = 0;
	static int justsent = 0;
	static uint32_t adcsentcnt = 0, talive = 0;
	volatile err_t err;
	int i;

	printf("Startudp:\n");
	osDelay(1000);

	/* get new pcbs */
	pcb = udp_new();
	if (pcb == NULL) {
		printf("startudp: udp_new failed!\n");
		for (;;)
			;
		return;
	}

	/* bind to any IP address on port 5000 */
	if (udp_bind(pcb, IP_ADDR_ANY, 5000) != ERR_OK) {
		printf("startudp: udp_bind failed!\n");
		for (;;)
			;
	}
	// use DNS instead of fixed ip?  lightning.bdars.org.au or something, we can override it locally without making changes to the code 
	// and make it easier to test at the club or something
	// getaddrinfo();
	
	
	/* set udp_echo_recv() as callback function for received packets */
//	udp_recv(pcb, myudp_recv, NULL);
	printf("UDP Destination 192.168.0.12\n");
	IP4_ADDR(&destip, 192, 168, 0, 12 /* 174 *//* 247 */);

	p1 = pbuf_alloc(PBUF_TRANSPORT, UDPBUFSIZE, PBUF_ROM);		// pk1 pbuf

	if (p1 == NULL) {
		printf("startudp: p1 buf_alloc failed!\n");
		for (;;)
			;
	}
	p1->payload = &(*pktbuf)[0];
//	p1->len = ADCBUFSIZE;

	p2 = pbuf_alloc(PBUF_TRANSPORT, UDPBUFSIZE, PBUF_ROM);		// pk1 pbuf
	if (p2 == NULL) {
		printf("startudp: p2 buf_alloc failed!\n");
		for (;;)
			;
	}
	p2->payload = &(*pktbuf)[(UDPBUFSIZE / 4)];	// half way along physical buffer

//	p2->len = ADCBUFSIZE;

	// trailing packet status packet
	ps = pbuf_alloc(PBUF_TRANSPORT, sizeof(statuspkt), PBUF_ROM);	// pks pbuf
	if (ps == NULL) {
		printf("startudp: ps buf_alloc failed!\n");
		for (;;)
			;
	}
	ps->payload = &statuspkt;	// point at status / GPS data

	osDelay(7000);

	for (i = 0; i < 10; i++) {
		err = udp_sendto(pcb, ps, &destip /*IP_ADDR_BROADCAST*/, 5000);
		statuspkt.udpcount++;
		if (err != ERR_OK) {
			printf("startudp: ps udp_sendto err %i\n", err);
			vTaskDelay(1999); //some delay!
		}
		osDelay(250);
	}

	while (1) {
//		p1 = pbuf_alloc(PBUF_TRANSPORT, sizeof(mypbuf), PBUF_ROM);		// header pbuf
//		p1->tot_len = sizeof(mypbuf);
//		vTaskDelay(1); //some delay!

		//    memcpy (p1->payload, (lastbuf == 0) ? testbuf : testbuf, ADCBUFLEN);

		while (myfullcomplete == lastsent) // last adc buffer == last sent buffer
			vTaskDelay(0);						// wait for adc finished
		lastsent = myfullcomplete;

		if (hangcount) {		// only send if adc threshold was exceeded

			HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_SET);	 // blue led on

			p = (lastsent & 1) ? p1 : p2;	// which dma buffer to send

			((uint8_t *) (p->payload))[3] = 0;	// pkt type

			((uint8_t *) (p->payload))[0] = statuspkt.udpcount & 0xff;
			((uint8_t *) (p->payload))[1] = (statuspkt.udpcount & 0xff00) >> 8;
			((uint8_t *) (p->payload))[2] = (statuspkt.udpcount & 0xff0000)
					>> 16;

			while (p->ref != 1) {		// old packet not finished with yet
				printf("******* p->ref = %d *******\n", p->ref);
			}

			err = udp_sendto(pcb, p, &destip, 5000);
			if (err != ERR_OK) {
				printf("startudp: p udp_sendto err %i\n", err);
				vTaskDelay(1999); //some delay!
			}
			statuspkt.adcpktssent++;		// UDP packet number
			justsent = 1;
			while (ps->ref != 1) {  // old packet not finished with yet
				;	// but we need wait to update the data packet next, so wait
			}
			statuspkt.udpcount++;		// UDP packet number
			HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_RESET); // blue led off
		} // if hangcount
		else	// nothing to send
		{
			if (justsent) {	// finished sending adc so now send a  GPS / status packet
				justsent = 0;
				while (ps->ref != 1) {  // old packet not finished with yet
					printf("******* ps->ref = %d *******\n", ps->ref);
				}
				((uint8_t *) (ps->payload))[3] = 1;	// status pkt type
				err = udp_sendto(pcb, ps, &destip /*IP_ADDR_BROADCAST*/, 5000);
				if (err != ERR_OK) {
					printf("startudp: ps udp_sendto err %i\n", err);
					vTaskDelay(1999); //some delay!
				}
				while (ps->ref != 1) {  // old packet not finished with yet
					;// but we need wait to update the data packet next, so wait
				}
				statuspkt.udpcount++;
				statuspkt.adcpktssent = 0;
			} else
			{
				if (t1sec != talive) {		// this is a temporary mech to do this...
					talive = t1sec;
					justsent = 1;		// force a status packet
				}
			}
		}
	} // forever while
}
//       pbuf_free(p1); //De-allocate packet buffer
//	   vTaskDelay( 100 ); //some delay!

