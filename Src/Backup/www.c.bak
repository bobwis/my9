/*
 * www.c
 *
 *  Created on: 6Jun.,2018
 *      Author: bob
 */
#include "main.h"
#include "stm32f7xx_hal.h"
#include "lwip.h"


/*--------------------------------------------------*/
// httpd server support
/*--------------------------------------------------*/

int httpd_cgi_handler(const char *uri, int count, char **http_cgi_params,
		char **http_cgi_param_vals)
{
	int i;

	printf("httpd_cgi_handler: uri=%s, count=%d\n", uri, count);
	for (i=0; i<count; i++) {
		printf("param %d, id=%c, va;=%c\n",i,(*http_cgi_params)[i],(*http_cgi_param_vals)[i] );
	}
}

int httpd_post_receive_data(void *connection, struct pbuf *p) {
	printf("httpd_post_receive_data: \n");
}

int httpd_post_begin(void *connection, const char *uri,
		const char *http_request, u16_t http_request_len, int content_len,
		char *response_uri, u16_t response_uri_len, u8_t *post_auto_wnd) {
	printf("httpd_post_begin: \n");
}

int httpd_post_finished(void *connection, char *response_uri,
		u16_t response_uri_len) {
	printf("httpd_post_finished: \n");
}

/* ---------------------------------------------- */
// http client
/* ---------------------------------------------- */

void  httpclient()
{
	volatile int result;
	uint32_t ip;
	int err;
	static ip_addr_t remoteip;
	char Page[] = "/api/Device/3333444S";
	char Postvars[] = {0}; // NULL

	static struct reply_struc {
	  u8_t Num;
	  char errorm[16];
	  char dunno[16];
	} reply;

	err = dnslookup("lightning.vk4ya.space", &remoteip);
	ip = remoteip.addr;
	printf("\nHTTP Target IP: %lu.%lu.%lu.%lu\n", ip & 0xff, (ip & 0xff00) >> 8,
			(ip & 0xff0000) >> 16, (ip & 0xff000000) >> 24);

	printf("calling hc_open\n");
	result = hc_open(remoteip, Page, 0 /*Postvars*/, &reply);
	printf("result=%d\n",result);
}
