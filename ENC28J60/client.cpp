#include "client.h"
//#include "dhcp.h"

#define PSTR(s) s

MAC_ADDRESS mymac = {0x78, 0x84, 0x00, 0xE1, 0x1C, 0x00};
IP_ADDRESS myip = {192, 168, 0, 100};
uint16_t myport = 1513;

// server settings - modify the service ip to your own server
IP_ADDRESS dest_ip = {127, 28, 11, 100};;	// Server IP
MAC_ADDRESS dest_mac = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}; // Server MAC
uint16_t dest_port = 1412;

//uint16_t data_length;
 
CLIENT_STATE client_state = IDLE;

const char * RequestData = "send me data\r\n";

uint8_t client_data_ready = 0;

uint16_t syn_ack_timeout = 0;

int bytes_to_read = 0;
int bytes_fetched = 0;


uint8_t buf[BUFFER_SIZE+1];

uint8_t databuf[30];

__IO uint8_t isConnected = 0;
uint8_t makeFinal = 0;

__IO uint32_t ack, seq; // I need to backup those because they may be lost when transfer new package


static void backup_ackseq(void)
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

void Client_init(IP_ADDRESS ClientAddr, uint16_t ClientPort)
{
	myip[0] = ClientAddr[0];
	myip[1] = ClientAddr[1];
	myip[2] = ClientAddr[2];
	myip[3] = ClientAddr[3];
	
	myport = ClientPort;
	
	#ifdef DEBUG
	USART1_sendStr((char*)"Initial Client\n");
	USART1_sendStr((char*)"Client IP Address: ");
	USART1_sendByte(myip[0], DEC);
	USART1_sendChar('.');
	USART1_sendByte(myip[1], DEC);
	USART1_sendChar('.');
	USART1_sendByte(myip[2], DEC);
	USART1_sendChar('.');
	USART1_sendByte(myip[3], DEC);
	USART1_sendChar('\n');
	USART1_sendStr((char*)"Client Port: ");
	USART1_sendNum(myport);
	USART1_sendChar('\n');
	#endif
		
	/*initialize enc28j60*/
	enc28j60Init(mymac);
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
	init_ip_arp_udp_tcp(mymac, myip);
}

void Client_connect(IP_ADDRESS ServerAddr, uint16_t ServerPort)
{
	// intialize varible;
	syn_ack_timeout =0;
	client_data_ready = 0;
	client_state = IDLE;
	
	dest_port = ServerPort;
	dest_ip[0] = ServerAddr[0];
	dest_ip[1] = ServerAddr[1];
	dest_ip[2] = ServerAddr[2];
	dest_ip[3] = ServerAddr[3];
}

void Client_makeFinal(void)
{
	makeFinal = 1;
}


void Client_ping(void)
{
	uint16_t plen = 0;
	uint16_t i = 0;
	
	// no data to send or DHCP is running
	if (client_data_ready == 0 && bytes_to_read == 0) // || DHCP_running == 1 )
		return;
	switch (client_state)
	{
		case IDLE:
			// initialize ARP, to get destination MAC address
			make_arp_request((uint8_t*)buf, dest_ip);
			client_state = ARP_SENT;
			break;
		
		case ARP_SENT:
			plen = enc28j60PacketReceive(BUFFER_SIZE, (uint8_t*)buf);

			// check destination ip address was found on network
			if ( plen!=0 )
			{
				// is reply my request?
				if (arp_packet_is_myreply_arp ( (uint8_t*)buf ) )
				{
					backup_ackseq();
					client_state = ARP_REPLY;
					syn_ack_timeout=0;
				}
			}
	// 		_delay_ms(10);
			syn_ack_timeout++;

			if(syn_ack_timeout == 1000) // 1s
			{  
				//timeout, server ip not found
				client_state = IDLE;
				client_data_ready =0;
				syn_ack_timeout=0;
				isConnected = 0;
				return;
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
				dest_mac,
				dest_ip
			);

			client_state = SYNC_SENT;
			break;
		
		case SYNC_SENT:
					
			syn_ack_timeout++;
			if (syn_ack_timeout > 3000) // 3s
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				client_state = IDLE;		// return to IDLE state
				client_data_ready = 0;		// client data lost
				isConnected = 0;
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
					dest_mac,
					dest_ip
				);
				isConnected = 1;
				client_state = SEND_DATA;
				syn_ack_timeout = 0;
			}
			break;
		case SEND_DATA:
			plen = fill_tcp_data_v2((uint8_t*)buf,0, (uint8_t*)databuf);
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
				dest_mac,
				dest_ip
			);
			if (bytes_to_read > 0) 
			{
				isConnected = 1;
				client_state = DATA_WAITING;
				client_data_ready = 0;
				syn_ack_timeout=0;
			} 
			else
			{
				isConnected = 1;
				syn_ack_timeout = 0;
				client_state = DATA_SENT;
			}
			break;
		case DATA_SENT:
			syn_ack_timeout++;
			if (syn_ack_timeout > 3000) // 3s
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				client_state = IDLE;		// return to IDLE state
				client_data_ready =0;		// client data sent
				isConnected = 0;
				break;
			}
			
			// after client send data to server, server response by send ACK to client
			plen = enc28j60PacketReceive(BUFFER_SIZE, (uint8_t*)buf);
			
			// check ip packet send to me or not?
			// accept ip packet only
			if (eth_type_is_ip_and_my_ip((uint8_t*)buf, plen) == 0)
				break;
			
			// check destination ip address was found on network
			if ( plen!=0 )
			{
				// is ACK?
				if ( buf [ TCP_FLAGS_P ] == (TCP_FLAG_ACK_V) )
				{
					backup_ackseq();
					if (makeFinal)
					{
						// client send RST to server
						tcp_client_send_packet (
							(uint8_t*)buf,
							dest_port,											 // destination port
							myport,								   // source port
							TCP_FLAG_RST_V | TCP_FLAG_ACK_V,				  // flag
							0,											  // (bool)maximum segment size
							0,											  // (bool)clear sequence ack number
							0,										   // 0=use old seq, seqack : 1=new seq,seqack no data : >1 new seq,seqack with data
							0,
							dest_mac,
							dest_ip
						);
						
						makeFinal = 0;
						client_state = IDLE;
						client_data_ready = 0;
						isConnected = 1; // just reset :D not lose connection
						syn_ack_timeout=0;
					}
					else
					{
						isConnected = 1;
						client_state = SEND_DATA;
						client_data_ready = 0;
						syn_ack_timeout=0;
					}
				}
			}
			
			break;
		case DATA_WAITING:
			syn_ack_timeout++;
			if (syn_ack_timeout > 3000) // 3s
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				bytes_to_read = 0;
				client_state = SEND_DATA;		// return to IDLE state
				client_data_ready =0;		// client data sent
				isConnected = 0;
				break;
			}
			
			// after client send data to server, server response by send ACK to client
			plen = enc28j60PacketReceive(BUFFER_SIZE, (uint8_t*)buf);
			
			// check ip packet send to me or not?
			// accept ip packet only
			if (eth_type_is_ip_and_my_ip((uint8_t*)buf, plen) == 0)
				break;
			
			// check destination ip address was found on network
			if ( plen!=0 )
			{
				if (buf[TCP_FLAGS_P] == (TCP_FLAG_ACK_V | TCP_FLAG_PUSH_V))
				{
					backup_ackseq();
					int total_length = (buf[IP_TOTLEN_H_P]<<8);
					total_length |= buf[IP_TOTLEN_L_P];
					bytes_fetched = total_length - IP_HEADER_LEN
						- TCP_HEADER_LEN_PLAIN;
					bytes_to_read -= bytes_fetched;
					isConnected = 1;
					client_state = DATA_RECEIVED;
					client_data_ready = 0;
					syn_ack_timeout=0;
				}
			}
			break;
			
		case DATA_RECEIVED:
			syn_ack_timeout++;
			if (syn_ack_timeout > 500) // 500ms
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				bytes_to_read = 0;
				client_state = SEND_DATA;		// return to IDLE state
				client_data_ready =0;		// client data sent
				isConnected = 1;
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
					dest_mac,
					dest_ip
				);
				if (makeFinal)
				{
					// client send RST to server
					tcp_client_send_packet (
						(uint8_t*)buf,
						dest_port,											 // destination port
						myport,								   // source port
						TCP_FLAG_RST_V | TCP_FLAG_ACK_V,				  // flag
						0,											  // (bool)maximum segment size
						0,											  // (bool)clear sequence ack number
						0,										   // 0=use old seq, seqack : 1=new seq,seqack no data : >1 new seq,seqack with data
						0,
						dest_mac,
						dest_ip
					);
					
					makeFinal = 0;
					client_state = IDLE;
					client_data_ready = 0;
					isConnected = 1; // just reset :D not lose connection
					syn_ack_timeout=0;
				}
				else
				{
					// go to send data state
					bytes_to_read = 0;
					isConnected = 1;
					client_state = SEND_DATA;
					client_data_ready = 0;
					syn_ack_timeout=0;
				}
			}
		break;
		case FIN_SENT:
			// wait ACK from Server
			// server responding by send to client ACK
			plen = enc28j60PacketReceive(BUFFER_SIZE, (uint8_t*)buf);
			
			// check ip packet send to me or not?
			// accept ip packet only
			if (eth_type_is_ip_and_my_ip((uint8_t*)buf, plen) == 0)
				break;
			
			if ( plen!=0 )
			{
				// is ACK?
				if ( buf [ TCP_FLAGS_P ] == (TCP_FLAG_ACK_V) )
				{
					client_state = END_SESSION;
					syn_ack_timeout=0;
				}
			}
			
			syn_ack_timeout++;
			if (syn_ack_timeout > 3000) // 3s
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				client_state = IDLE;		// return to IDLE state
				client_data_ready =0;		// client data sent
				isConnected = 0;
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
						dest_mac,
						dest_ip
					);
					client_state = IDLE;		// return to IDLE state
					syn_ack_timeout = 0;
					client_data_ready =0;		// client data sent
				}
			}
			
			syn_ack_timeout++;
			if (syn_ack_timeout > 3000) // 3s
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				client_state = IDLE;		// return to IDLE state
				client_data_ready =0;		// client data sent
				isConnected = 0;
				break;
			}
			break;
		default:
			syn_ack_timeout = 0;
			client_state = IDLE;		// return to IDLE state
			client_data_ready =0;		// client data sent
			isConnected = 0;
			break;
	}
}

void Client_sendStr(char * str)
{
	int i = 0;
	int len = 0;
	unsigned int tmpT= millis();
	
	// wait for prevous sent completed
	while(client_data_ready || bytes_to_read > 0)
	{
		if (millis() - tmpT > 300) // 300 ms timeout
			return;
	}
	
	
	while(*str != '\0')
	{
		databuf[i++] = (uint8_t)*str++;
		len++;
	}
	
	databuf[len] = '#';
	client_data_ready = 1;
}

void Client_send(uint8_t * data, int length)
{
	int i;
	unsigned int tmpT = millis();
	
	// wait for prevous sent completed
	while(client_data_ready || bytes_to_read > 0)
	{
		if (millis() - tmpT > 300) // 300 ms timeout
			return;
	}
	
	for (i = 0; i < length; i++)
		databuf[i] = data[i];
	databuf[length] = '#';
	client_data_ready = 1;
}

int Client_read(uint8_t * buffer, int length)
{
	bytes_fetched = 0;
	int i = 0;
	
	
	if (bytes_to_read > 0) // wait or skip?
		while(bytes_to_read == 0); // wait
	
	
	
	// to make sure the server not send data when client is not ready
	// the client must active :D
	Client_send((uint8_t *)RequestData, 14);
	
	bytes_to_read = length;
	// wait the sending finished
	while(client_data_ready != 0);
	
	
	// loop here or ping?
	//// ping
	//// return length - bytes_to_read;
	// loop
	while (bytes_to_read == length);
	
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
//	DHCP_init(mymac);
//	DHCP_getIP(buf, myip, 100);
}
