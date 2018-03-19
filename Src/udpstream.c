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
#include "ip_addr.h"

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

struct ip4_addr destip;		// dst ipv4 address
static int ip_ready = 0;

void dnsfound(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {

	destip.addr = ipaddr->addr;
	ip_ready = 1;
}

void startudp() {
	struct udp_pcb *pcb;
	volatile struct pbuf *p, *p1, *p2, *ps;

	uint32_t lastsent = 0;
	uint32_t ip = 0;;
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
	

//	udp_recv(pcb, myudp_recv, NULL);
#if 0
	printf("UDP Destination 192.168.0.12\n");
	IP4_ADDR(&destip, 192, 168, 0, 12 /* 174 *//* 247 */);

	printf("192 destip=0x%08x\n",destip);
#endif

// set destination server IP using DNS lookup

	printf("DNS Resolving %s ... ", SERVER_DESTINATION);
	if (err = dns_gethostbyname(SERVER_DESTINATION, &destip, dnsfound, 0)) {
		switch (err) {
	case ERR_OK:		// a cached result
		ip_ready = 1;
		break;
	case ERR_INPROGRESS:		// a callback result to dnsfound if it find it
		printf("gethostbyname INPROGRESS");
		for (i = 0; i < 20; i++) {
			osDelay(1000);		// give it 10 seconds
			printf(".");
			if (ip_ready)
				break;
		}
		if (!(ip_ready)) {
			printf("****** DNS Lookup Failed *******\n");
			return;
		}
		break;
	default:
		printf("****** gethostbyname failed *****\n ");
		return;
		break;
		}
	}

	ip = destip.addr;
	printf("\nTarget IP at %u.%u.%u.%u\n",ip & 0xff,(ip & 0xff00)>>8,(ip & 0xff0000)>>16,(ip & 0xff000000)>>24);

	p1 = pbuf_alloc(PBUF_TRANSPORT, UDPBUFSIZE, PBUF_ROM);		// pk1 pbuf

	if (p1 == NULL) {
		printf("startudp: p1 buf_alloc failed!\n");
		return;
	}
	p1->payload = &(*pktbuf)[0];
//	p1->len = ADCBUFSIZE;

	p2 = pbuf_alloc(PBUF_TRANSPORT, UDPBUFSIZE, PBUF_ROM);		// pk1 pbuf
	if (p2 == NULL) {
		printf("startudp: p2 buf_alloc failed!\n");
		return;
	}
	p2->payload = &(*pktbuf)[(UDPBUFSIZE / 4)];	// half way along physical buffer

//	p2->len = ADCBUFSIZE;

	// trailing packet status packet
	ps = pbuf_alloc(PBUF_TRANSPORT, sizeof(statuspkt), PBUF_ROM);	// pks pbuf
	if (ps == NULL) {
		printf("startudp: ps buf_alloc failed!\n");
		return;
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

	statuspkt.reserved1 = 0x11111111;
	statuspkt.reserved2 = 0x22222222;
	statuspkt.reserved3 = 0x33333333;
	statuspkt.reserved4 = 0x44444444;
	statuspkt.reserved5 = 0x55555555;
	statuspkt.telltale1 = 0xDEC0EDFE; //  0xFEEDC0DE marker at the end of each status packet

	netup = 1;		// this is incomplete - it should be set by the phys layer also
	printf("Starting UDP Stream loop\n");
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
			((uint8_t *) (p->payload))[2] = (statuspkt.udpcount & 0xff0000)>> 16;

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
			statuspkt.udpcount++;		// UDP packet number - global var used by all
			HAL_GPIO_WritePin(GPIOB, LD2_Pin, GPIO_PIN_RESET); // blue led off
		} // if hangcount
		else	// no adc sample data to send
		{
			if (justsent) {	// just finished sending adc so now send a  GPS / status packet
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
					; // but we need wait to update the data packet next, so wait
				}
				statuspkt.udpcount++;
				statuspkt.adcpktssent = 0;
			} else {
				if (t1sec != talive) {// this is a temporary mech to send timed status pkts...
					talive = t1sec;
					while (ps->ref != 1) {  // old packet not finished with yet
						printf("******* ps->ref = %d *******\n", ps->ref);
					}
					((uint8_t *) (ps->payload))[3] = 2;	// timed status pkt type
					err = udp_sendto(pcb, ps, &destip /*IP_ADDR_BROADCAST*/, 5000);
					if (err != ERR_OK) {
						printf("startudp: ps udp_sendto err %i\n", err);
						vTaskDelay(1999); //some delay!
					}
					while (ps->ref != 1) {  // old packet not finished with yet
						; // but we need wait to update the data packet next, so wait
					statuspkt.udpcount++;
					}
				}
			}
		}
	} // forever while
}
//       pbuf_free(p1); //De-allocate packet buffer
//	   vTaskDelay( 100 ); //some delay!

