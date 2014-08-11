#ifndef _Client_h
#define _Client_h

#define BUFFER_SIZE 1000
typedef char IP_ADDRESS[4];
typedef char MAC_ADDRESS[6];

typedef enum 
{  
   IDLE, ARP_SENT, ARP_REPLY, SYNC_SENT, SEND_DATA, DATA_SENT, 
	DATA_WAITING, END_SESSION, FIN_SENT, DATA_RECEIVED, RESET_CONNECT
} CLIENT_STATE;

extern unsigned int ack, seq;

void Client_init(IP_ADDRESS ClientAddr, int ClientPort);
void Client_connect(IP_ADDRESS ServerAddr, int ServerPort);
void Client_sendStr(char *);
void Client_makeFinal(void);
void Client_ping(void);

int Client_read(char * buffer, int length); 

//// for test only
//void Client_getIP(void);


#endif
