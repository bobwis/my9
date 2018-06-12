/*
 printf("******* ps->ref = %d *******\n", ps->ref); * udpstream.c
 *
 *  Created on: 22Dec.,2017
 *      Author: bob
 */
#include "lwip.h"
#include "udpstream.h"
#include "adcstream.h"
#include "mydebug.h"
#include "FreeRTOS.h"
#include "neo7m.h"
#include "ip_addr.h"
#include "lwip/dns.h"
#include "lwip/prot/dns.h"

extern uint32_t t1sec;
uint8_t gpslocked = 0;
uint8_t epochvalid = 0;

static struct ip4_addr udpdestip;		// udp dst ipv4 address
static uint32_t ip_ready = 0;


void myudp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p,
		struct ip_addr *addr, u16_t port) {
	if (p != NULL) {
		/* send received packet back to sender */
		udp_sendto(pcb, p, addr, port);
		/* free the pbuf */
		pbuf_free(p);
	}
}

//
// send a status packet
//
inline void sendstatus(int stype, struct pbuf *ps, struct udp_pcb *pcb,
		uint8_t batchid) {

	volatile err_t err;

	statuspkt.auxstatus1 = (statuspkt.auxstatus1 & 0xffff0000)
			| (((jabber & 0xff) << 8) | batchid);

	while (ps->ref != 1) { // old packet not finished with yet
		printf("******* timed status1: ps->ref = %d *******\n", ps->ref);
	}

	((uint8_t *) (ps->payload))[3] = stype; // timed status pkt type

	err = udp_sendto(pcb, ps, &udpdestip, UDP_PORT_NO);
	if (err != ERR_OK) {
		printf("startudp: ps udp_sendto err %i\n", err);
		vTaskDelay(1999); //some delay!
	}
	while (ps->ref != 1) { // old packet not finished with yet
		printf("******* timed status2: ps->ref = %d *******\n", ps->ref);
		vTaskDelay(0); // but we need wait to update the data packet next, so wait
	}
	statuspkt.udpcount++;
}

//
// send timed status packet if is time
//
void sendtimedstatus(struct pbuf *ps, struct udp_pcb *pcb, uint8_t batchid) {
	static uint32_t talive = 0;

#ifdef TESTING
	if ((t1sec != talive)) { //} && (t1sec % 2 == 0)) { // this is a temporary mech to send timed status pkts...
		talive = t1sec;

#else
		if ((t1sec != talive) && (t1sec % 120 == 0)) { // this is a temporary mech to send timed status pkts...
			talive = t1sec;
#endif
		sendstatus(TIMED, ps, pcb, batchid);
	}
}

// Delayed DNS lookup result callback

void dnsfound(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {

	ip_ready = ipaddr->addr;
}

// set destination server IP using DNS lookup
int dnslookup(char *name, struct ip4_addr *ip) {
	int i, err = 0;

	printf("DNS Resolving %s ", name);
	ip_ready = 0;
	err = dns_gethostbyname(name, ip, dnsfound, 0);

	switch (err) {
	case ERR_OK:		// a cached result already in *ip.addr
		break;
	case ERR_INPROGRESS:	// a callback result to dnsfound if it finds it
		printf("gethostbyname INPROGRESS");
		for (i = 0; i < 20; i++) {
			osDelay(1000);		// give it 20 seconds
			printf(".");
			if (ip_ready) {
				ip->addr = ip_ready;
				return(ERR_OK);
			}
		if (err == ERR_OK)
			break;
		} // falls through on timeout
	default:
		printf("****** gethostbyname failed *****\n ");
		break;
	}
	return (err);
}

void startudp() {
	struct udp_pcb *pcb;
	struct pbuf *p, *p1, *p2, *ps;
	uint32_t ulNotificationValue = 0;
	const TickType_t xMaxBlockTime = pdMS_TO_TICKS(1000);
	uint32_t ip = 0;
	volatile err_t err;
	int i;

	static uint8_t lastadcbatchid = 0;

	printf("Startudp:\n");
	/* Store the handle of the calling task. */
	xTaskToNotify = xTaskGetCurrentTaskHandle();
	osDelay(1000);

	/* get new pcbs */
	pcb = udp_new();
	if (pcb == NULL) {
		printf("startudp: udp_new failed!\n");
		for (;;)
			;
		return;
	}

	/* bind to any IP address on port UDP_PORT_NO */
	if (udp_bind(pcb, IP_ADDR_ANY, UDP_PORT_NO) != ERR_OK) {
		printf("startudp: udp_bind failed!\n");
		for (;;)
			;
	}

//	udp_recv(pcb, myudp_recv, NULL);

// set UDP destination server IP using DNS lookup
	err = dnslookup(SERVER_DESTINATION, &udpdestip);
	ip = udpdestip.addr;
	printf("\nUDP Target IP: %lu.%lu.%lu.%lu\n", ip & 0xff, (ip & 0xff00) >> 8,
			(ip & 0xff0000) >> 16, (ip & 0xff000000) >> 24);

	p1 = pbuf_alloc(PBUF_TRANSPORT, UDPBUFSIZE, PBUF_REF /* PBUF_ROM */); // pk1 pbuf

	if (p1 == NULL) {
		printf("startudp: p1 buf_alloc failed!\n");
		return;
	}
	p1->payload = &(*pktbuf)[0];
//	p1->len = ADCBUFSIZE;

	p2 = pbuf_alloc(PBUF_TRANSPORT, UDPBUFSIZE, PBUF_REF /* PBUF_ROM */); // pk1 pbuf
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

	statuspkt.auxstatus1 = 0;
	statuspkt.adcudpover = 0;		// debug use count overruns
	statuspkt.trigcount = 0;		// debug use adc trigger count
	statuspkt.udpsent = 0;	// debug use adc udp sample packet sent count
	statuspkt.telltale1 = 0xDEC0EDFE; //  0xFEEDC0DE marker at the end of each status packet

	netup = 1; // this is incomplete - it should be set by the phys layer also
	printf("Starting UDP Stream loop\n");

	while (1) {

//		p1 = pbuf_alloc(PBUF_TRANSPORT, sizeof(mypbuf), PBUF_ROM);		// header pbuf
//		p1->tot_len = sizeof(mypbuf);
//		vTaskDelay(1); //some delay!

		//    memcpy (p1->payload, (lastbuf == 0) ? testbuf : testbuf, ADCBUFLEN);

		/* Wait to be notified */
		ulNotificationValue = ulTaskNotifyTake( pdTRUE, xMaxBlockTime);
#if 0
		if (ulNotificationValue == 1) {
			/* The transmission ended as expected. */
		} else {
			/* The call to ulTaskNotifyTake() timed out. */
			printf("ulNotificationValue = %d\n",ulNotificationValue );
		}
#endif
		/* send end of sequence status packet if end of batch sequence */
		if ((sendendstatus > 0) && (jabber == 0)) {
			sendstatus(ENDSEQ, ps, pcb, lastadcbatchid); // send end of seq status
			sendendstatus = 0;	// cancel the flag
			lastadcbatchid = adcbatchid;
			statuspkt.adcpktssent = 0;	// end of sequence so start again at 0
		}

		/* if we have a trigger, send a sample packet */
		if ((sigsend) && (gpslocked)) { // only send if adc threshold was exceeded
			// and GPS is locked

			p = (dmabufno) ? p2 : p1; // which dma buffer to send, dmabuf is last filled buffer, 0 or 1

			((uint8_t *) (p->payload))[3] = 0;	// pkt type

			((uint8_t *) (p->payload))[0] = statuspkt.udpcount & 0xff;
			((uint8_t *) (p->payload))[1] = (statuspkt.udpcount & 0xff00) >> 8;
			((uint8_t *) (p->payload))[2] = (statuspkt.udpcount & 0xff0000)
					>> 16;

			while (p->ref != 1) {	// old packet not finished with yet
				printf("*******send sample failed p->ref = %d *******\n",
						p->ref);
			}

			if (jabber == 0) {		// don't actually send it if jabbering
				err = udp_sendto(pcb, p, &udpdestip, UDP_PORT_NO);
				statuspkt.udpsent++;	// debug no of sample packets set
				if (err != ERR_OK) {
					printf("startudp: p udp_sendto err %i\n", err);
					vTaskDelay(1999); //some delay!
				}

				statuspkt.adcpktssent++;	// UDP sample packet counterr
				statuspkt.udpcount++;		// UDP packet number
				while (ps->ref != 1) {  // old packet not finished with yet
					printf("******* end sample status: ps->ref = %d *******\n",
							ps->ref);
					vTaskDelay(0); // but we need wait to update the data packet next, so wait
				}
			} else {
				sendtimedstatus(ps, pcb, lastadcbatchid); // on jabber, timed status sending masked by sigsend
			}
			sigsend = 0;		// assume its sent
		} // if sigsend
		else {		// no adc sample to send, so send timed status
			sendtimedstatus(ps, pcb, lastadcbatchid);
		}
	} // forever while
}

//       pbuf_free(p1); //De-allocate packet buffer

