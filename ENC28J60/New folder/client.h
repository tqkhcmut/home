#ifndef Client_h
#define Client_h

#ifdef __cplusplus
extern "C" {
#endif
	
#include "ENC28J60/enc28j60.h"
#include "ENC28J60/ip_arp_udp_tcp.h"
#include "ENC28J60/net.h"
#include <inttypes.h>
	#include "delay/delay.h"


#define BUFFER_SIZE 1500
typedef uint8_t IP_ADDRESS[4];
typedef uint8_t MAC_ADDRESS[6];

enum CLIENT_STATE
{  
   IDLE, ARP_SENT, ARP_REPLY, SYNC_SENT, SEND_DATA, END_SESSION
};

void Client_connect(IP_ADDRESS ServerAddr, uint16_t ServerPort);
void Client_sent(uint8_t * data, uint8_t length);
void Client_ping(void);

#ifdef __cplusplus
}
#endif

#endif
