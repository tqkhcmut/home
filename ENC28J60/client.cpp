#include "client.h"
#include "ENC28J60/enc28j60.h"
#include "ENC28J60/ip_arp_udp_tcp.h"
#include "ENC28J60/net.h"
#include "delay/delay.h"
#include "queue.h"

#define PSTR(s) s
#define TIME_OUT 100
#define RequestData "send me data\r\n"

Queue sendDataQueue;

MAC_ADDRESS mymac = {0x78, 0x84, 0x00, 0xE1, 0x1C, 0x00};
IP_ADDRESS myip = {192, 168, 0, 100};
int myport = 1513;

// server settings - modify the service ip to your own server
IP_ADDRESS dest_ip = {0, 0, 0, 0};	// Server IP
MAC_ADDRESS dest_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}; // Server MAC
int dest_port = 0;
 
CLIENT_STATE client_state = IDLE;

char client_initialized = 0;
int syn_ack_timeout = 0;
int bytes_to_read = 0;
int bytes_fetched = 0;
char buf[BUFFER_SIZE+1];
char makeFinal = 0;

unsigned int ack, seq; // I need to backup those because they may be lost when transfer new package

void backup_ackseq(void)
{
	ack = 0;
	seq = 0;
	
	ack |= buf[TCP_SEQACK_P];
	ack <<= 8;
	ack |= buf[TCP_SEQACK_P+1];
	ack <<= 8;
	ack |= buf[TCP_SEQACK_P+2];
	ack <<= 8;
	ack |= buf[TCP_SEQACK_P+3];
	
	
	seq |= buf[TCP_SEQ_P];
	seq <<= 8;
	seq |= buf[TCP_SEQ_P+1];
	seq <<= 8;
	seq |= buf[TCP_SEQ_P+2];
	seq <<= 8;
	seq |= buf[TCP_SEQ_P+3];
}

void Client_init(IP_ADDRESS ClientAddr, int ClientPort)
{
	myip[0] = ClientAddr[0];
	myip[1] = ClientAddr[1];
	myip[2] = ClientAddr[2];
	myip[3] = ClientAddr[3];
	
	myport = ClientPort;
		
	/*initialize enc28j60*/
	enc28j60Init((uint8_t*)mymac);
	enc28j60clkout(2); // change clkout from 6.25MHz to 12.5MHz
	_delay_ms(10);

	/* Magjack leds configuration, see enc28j60 datasheet, page 11 */
	// LEDA=greed LEDB=yellow
	//
	// 0x880 is PHLCON LEDB=on, LEDA=on
	// enc28j60PhyWrite(PHLCON,0b0000 1000 1000 00 00);
	enc28j60PhyWrite(PHLCON,0x880);
	_delay_ms(500);
	//
	// 0x990 is PHLCON LEDB=off, LEDA=off
	// enc28j60PhyWrite(PHLCON,0b0000 1001 1001 00 00);
	enc28j60PhyWrite(PHLCON,0x990);
	_delay_ms(500);
	//
	// 0x880 is PHLCON LEDB=on, LEDA=on
	// enc28j60PhyWrite(PHLCON,0b0000 1000 1000 00 00);
	enc28j60PhyWrite(PHLCON,0x880);
	_delay_ms(500);
	//
	// 0x990 is PHLCON LEDB=off, LEDA=off
	// enc28j60PhyWrite(PHLCON,0b0000 1001 1001 00 00);
	enc28j60PhyWrite(PHLCON,0x990);
	_delay_ms(500);
	//
	// 0x476 is PHLCON LEDA=links status, LEDB=receive/transmit
	// enc28j60PhyWrite(PHLCON,0b0000 0100 0111 01 10);
	enc28j60PhyWrite(PHLCON,0x476);
	_delay_ms(100);

	//init the ethernet/ip layer:
	init_ip_arp_udp_tcp((uint8_t*)mymac, (uint8_t *)myip);
	
	// initial queue
	sendDataQueue.Init();
	// set flag
	client_initialized = 1;
}

void Client_connect(IP_ADDRESS ServerAddr, int ServerPort)
{
	// intialize varible;
	syn_ack_timeout =0;
	client_state = IDLE;
	
	dest_port = ServerPort;
	dest_ip[0] = ServerAddr[0];
	dest_ip[1] = ServerAddr[1];
	dest_ip[2] = ServerAddr[2];
	dest_ip[3] = ServerAddr[3];
	
	// set flag
	client_initialized = 2;
}

void Client_makeFinal(void)
{
	makeFinal = 1;
}


void Client_ping(void)
{
	int i, plen = 0;
	
	if (client_initialized == 0) // make sure client ready, preventing some crash or dead code
		return;
	if (client_initialized == 1) // not config server ip
		return;

	switch (client_state)
	{
		case IDLE:
			// initialize ARP, to get destination MAC address
			make_arp_request((uint8_t*)buf, (uint8_t*)dest_ip);
			client_state = ARP_SENT;
			break;
		
		case ARP_SENT:
			plen = enc28j60PacketReceive(BUFFER_SIZE, (uint8_t*)buf);

			// check destination ip address was found on network
			if ( plen!=0 )
			{
				// is reply my request?
				if (arp_packet_is_myreply_arp ((uint8_t*)buf))
				{
					client_state = ARP_REPLY;
					syn_ack_timeout=0;
				}
			}
			syn_ack_timeout++;

			if(syn_ack_timeout >= TIME_OUT)
			{  
				//timeout, server ip not found
				client_state = IDLE;
				syn_ack_timeout=0;
				break;
			}
			break;
		
		case ARP_REPLY:
			// save dest mac
			for(i=0; i<6; i++)
			{
				dest_mac[i] = buf[ETH_SRC_MAC+i];
			}
			
			// open TCP connection session
			// send syn message in three-way handshake
			tcp_client_send_packet (
				(uint8_t*)buf,
				dest_port,
				myport,
				TCP_FLAG_SYN_V,				 // flag
				1,											  // (bool)maximum segment size
				1,											  // (bool)clear sequence ack number
				0,											  // 0=use old seq, seqack : 1=new seq,seqack no data : new seq,seqack with data
				0,											  // tcp data length
				(uint8_t*)dest_mac,
				(uint8_t*)dest_ip
			);

			client_state = SYNC_SENT;
			break;
		
		case SYNC_SENT:
					
			syn_ack_timeout++;
			if (syn_ack_timeout > TIME_OUT)
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				client_state = IDLE;		// return to IDLE state
				break;
			}
			
			// server response with ACK and SYN flag
			// three-way handshake
			plen = enc28j60PacketReceive(BUFFER_SIZE, (uint8_t*)buf);
			
			// if no new packet incoming, wait for it
			if ( plen == 0 )
				break;
			
			// else
			// check ip packet send to me or not?
			// accept ip packet only
			if (eth_type_is_ip_and_my_ip((uint8_t*)buf, plen) == 0)
				break;
			
			// else
			// check SYNACK flag, after client send SYN server response by send SYNACK to client
			if (buf[TCP_FLAGS_P] == (TCP_FLAG_SYN_V | TCP_FLAG_ACK_V))
			{
				backup_ackseq();
			
				// client send ACK flag to open TCP session
				tcp_client_send_packet (
					(uint8_t*)buf,
					dest_port,
					myport,
					TCP_FLAG_ACK_V,				 // flag
					0,											  // (bool)maximum segment size
					0,											  // (bool)clear sequence ack number
					1,											  // 0=use old seq, seqack : 1=new seq,seqack no data : new seq,seqack with data
					0,											  // tcp data length
					(uint8_t*)dest_mac,
					(uint8_t*)dest_ip
				);
				client_state = SEND_DATA;
				syn_ack_timeout = 0;
			}
			break;
		case SEND_DATA:
			if (sendDataQueue.Size() == 0)
				break;
			plen = fill_tcp_data_v2((uint8_t*)buf,0, (uint8_t*)sendDataQueue.Dequeue());
			// send packet with PUSH-ACK
			tcp_client_send_packet (
				(uint8_t*)buf,
				dest_port,											 // destination port
				myport,								   // source port
				TCP_FLAG_ACK_V | TCP_FLAG_PUSH_V,						// flag
				0,											  // (bool)maximum segment size
				0,											  // (bool)clear sequence ack number
				0,											  // 0=use old seq, seqack : 1=new seq,seqack no data : >1 new seq,seqack with data
				plen,						   // tcp data length
				(uint8_t*)dest_mac,
				(uint8_t*)dest_ip
			);
			if (bytes_to_read > 0) 
			{
				client_state = DATA_WAITING;
				syn_ack_timeout=0;
			} 
			else
			{
				syn_ack_timeout = 0;
				client_state = DATA_SENT;
			}
			break;
		case DATA_SENT:
			syn_ack_timeout++;
			if (syn_ack_timeout > TIME_OUT)
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				client_state = RESET_CONNECT;		// return to RESET_CONECT state
				break;
			}
			
			// after client send data to server, server response by send ACK to client
			plen = enc28j60PacketReceive(BUFFER_SIZE, (uint8_t*)buf);
			
			if (plen == 0)
				break;
			
			// check ip packet send to me or not?
			// accept ip packet only
			if (eth_type_is_ip_and_my_ip((uint8_t*)buf, plen) == 0)
				break;
			
			// is ACK?
			if ( buf [ TCP_FLAGS_P ] == (TCP_FLAG_ACK_V) )
			{
				backup_ackseq();
				if (makeFinal)
				{						
					makeFinal = 0;
					client_state = RESET_CONNECT;
					syn_ack_timeout=0;
				}
				else
				{
					client_state = SEND_DATA; // come back to send new data
					syn_ack_timeout=0;
				}
			}
			
			break;
		case DATA_WAITING:
			syn_ack_timeout++;
			if (syn_ack_timeout > TIME_OUT)
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				bytes_to_read = 0;
				client_state = RESET_CONNECT;		// return to RESET_CONNECT state
				break;
			}
			
			// read data package
			plen = enc28j60PacketReceive(BUFFER_SIZE, (uint8_t*)buf);
			
			if (plen == 0) // no package
				break; 
			
			// check ip packet send to me or not?
			// accept ip packet only
			if (eth_type_is_ip_and_my_ip((uint8_t*)buf, plen) == 0)
				break;
			
			if (buf[TCP_FLAGS_P] == (TCP_FLAG_ACK_V | TCP_FLAG_PUSH_V))
			{
				backup_ackseq();
				int total_length = (buf[IP_TOTLEN_H_P]<<8);
				total_length |= buf[IP_TOTLEN_L_P];
				bytes_fetched = total_length - IP_HEADER_LEN
					- TCP_HEADER_LEN_PLAIN;
				bytes_to_read -= bytes_fetched;
				client_state = DATA_RECEIVED;
				syn_ack_timeout=0;
			}
			break;
			
		case DATA_RECEIVED:
			syn_ack_timeout++;
			if (syn_ack_timeout > TIME_OUT)
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				bytes_to_read = 0;
				client_state = SEND_DATA;		// return to IDLE state
				break;
			}
		
			if (bytes_to_read == -100) 
			// data have been buffered, 1s timeout
			{
				int total_length = (buf[IP_TOTLEN_H_P]<<8);
					total_length |= buf[IP_TOTLEN_L_P];
				// reply server by ack
				tcp_client_send_packet (
					(uint8_t*)buf,
					dest_port,
					myport,
					TCP_FLAG_ACK_V,				 // flag
					0,											  // (bool)maximum segment size
					0,											  // (bool)clear sequence ack number
					bytes_fetched,				  // 0=use old seq, seqack : 1=new seq,seqack no data : new seq,seqack with data
					0,											  // tcp data length
					(uint8_t*)dest_mac,
					(uint8_t*)dest_ip
				);
				if (makeFinal)
				{					
					bytes_to_read = 0;
					makeFinal = 0;
					client_state = RESET_CONNECT;
					syn_ack_timeout=0;
				}
				else
				{
					// go to send data state
					bytes_to_read = 0;
					client_state = SEND_DATA;
					syn_ack_timeout=0;
				}
			}
		break;
		case RESET_CONNECT:
			// client send RST to server
			tcp_client_send_packet(
				(uint8_t*)buf,
				dest_port,											 // destination port
				myport,								   // source port
				TCP_FLAG_RST_V | TCP_FLAG_ACK_V,				  // flag
				0,											  // (bool)maximum segment size
				0,											  // (bool)clear sequence ack number
				0,										   // 0=use old seq, seqack : 1=new seq,seqack no data : >1 new seq,seqack with data
				0,
				(uint8_t*)dest_mac,
				(uint8_t*)dest_ip
			);
			bytes_to_read = 0;
			makeFinal = 0;
			client_state = IDLE;
			syn_ack_timeout=0;
			break;
		case FIN_SENT:
			// wait ACK from Server
			// server responding by send to client ACK
			plen = enc28j60PacketReceive(BUFFER_SIZE, (uint8_t*)buf);
		
			if (plen == 0)
				break;
			
			// check ip packet send to me or not?
			// accept ip packet only
			if (eth_type_is_ip_and_my_ip((uint8_t*)buf, plen) == 0)
				break;
			
			// is ACK?
			if ( buf [ TCP_FLAGS_P ] == (TCP_FLAG_ACK_V) )
			{
				client_state = END_SESSION;
				syn_ack_timeout=0;
			}
			
			syn_ack_timeout++;
			if (syn_ack_timeout > TIME_OUT)
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				client_state = IDLE;		// return to IDLE state
				break;
			}
			break;
			
		case END_SESSION:
			
			// wait server send to client FIN package
			plen = enc28j60PacketReceive(BUFFER_SIZE, (uint8_t*)buf);
			
			if ( plen!=0 )
			{
				// is FIN?
				if ( buf [ TCP_FLAGS_P ] == (TCP_FLAG_FIN_V) )
				{
					// send ACK with seqack = 1
					tcp_client_send_packet(
						(uint8_t*)buf,
						dest_port,											 // destination port
						myport,								   // source port
						TCP_FLAG_ACK_V,				 // flag
						0,											  // (bool)maximum segment size
						0,											  // (bool)clear sequence ack number
						1,											  // 0=use old seq, seqack : 1=new seq,seqack no data : >1 new seq,seqack with data
						0,
						(uint8_t*)dest_mac,
						(uint8_t*)dest_ip
					);
					client_state = IDLE;		// return to IDLE state
					syn_ack_timeout = 0;
//					client_data_ready =0;		// client data sent
				}
			}
			
			syn_ack_timeout++;
			if (syn_ack_timeout > TIME_OUT)
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				client_state = IDLE;		// return to IDLE state
				break;
			}
			break;
		default:
			bytes_to_read = 0;
			syn_ack_timeout = 0;
			client_state = IDLE;		// return to IDLE state
			break;
	}
}

void Client_sendStr(char * str)
{
	sendDataQueue.Enqueue(str);
}

int Client_read(char * buffer, int length)
{
	int tmpTime = millis();
	bytes_fetched = 0;
	int i = 0;
	
	
	while(bytes_to_read > 0)
	{
		if (millis() - tmpTime > 500)
		{
			bytes_to_read = 0;
			return -1;
		}
	}
	
	Client_sendStr((char*)RequestData);
	
	bytes_to_read = length;
	tmpTime = millis();
	while (bytes_to_read == length )
	{
		if (millis() - tmpTime > 100) // 100 ms timeout
		{
			bytes_to_read = 0;
			return 0;
		}
	}
	
	bytes_fetched = length - bytes_to_read;
	for (i = 0; i < bytes_fetched; i++)
	{
		buffer[i] = buf[TCP_DATA_P+i];
	}
	bytes_to_read = -100;
	return bytes_fetched;
}

void Client_getIP(void) 
{
}
