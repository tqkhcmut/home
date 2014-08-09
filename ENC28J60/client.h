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
	
	extern __IO uint8_t isConnected;
	extern __IO uint32_t ack, seq; 

enum CLIENT_STATE
{  
   IDLE, ARP_SENT, ARP_REPLY, SYNC_SENT, SEND_DATA, DATA_SENT, DATA_WAITING, END_SESSION, FIN_SENT, DATA_RECEIVED
};

void Client_init(IP_ADDRESS ClientAddr, uint16_t ClientPort);
void Client_connect(IP_ADDRESS ServerAddr, uint16_t ServerPort);
void Client_sendStr(char *);
void Client_send(uint8_t * data, int length);
void Client_makeFinal(void);
void Client_ping(void);

extern const char * RequestData;
//
// the buffer length make sure there are no overload on buffer
int Client_read(uint8_t * buffer, int length); 

// for test only
void Client_getIP(void);

#ifdef __cplusplus
}
#endif

#endif
