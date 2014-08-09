#include "client.h"

#define PSTR(s) s

MAC_ADDRESS mymac;
IP_ADDRESS myip;
uint16_t myport;

// server settings - modify the service ip to your own server
IP_ADDRESS dest_ip;	// Server IP
MAC_ADDRESS dest_mac; // Server MAC
uint16_t dest_port;

uint16_t data_length;
 
CLIENT_STATE client_state;

uint8_t client_data_ready;

uint16_t syn_ack_timeout; // = 0;


uint8_t buf[BUFFER_SIZE+1];

uint8_t databuf[30];

void Client_connect(IP_ADDRESS ServerAddr, uint16_t ServerPort)
{
	mymac[0] = 0x78;
	mymac[1] = 0x84;
	mymac[2] = 0x00;
	mymac[3] = 0xE1;
	mymac[4] = 0x1C;
	mymac[5] = 0x00;
	
	myip[0] = 192;
	myip[1] = 168;
	myip[2] = 1;
	myip[3] = 107;
	
	myport = 1010;
		
	data_length = 0;
	
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
	init_ip_arp_udp_tcp(mymac,myip,myport);

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

void Client_ping(void)
{
	uint16_t plen = 0;
	uint16_t i = 0;
	
	if (client_data_ready == 0 || data_length == 0)  
		return;	 // nothing to send
	
	switch (client_state)
	{
		case IDLE:
			// initialize ARP, to get destination MAC address
			make_arp_request(buf, dest_ip);
			client_state = ARP_SENT;
			break;
		
		case ARP_SENT:
			plen = enc28j60PacketReceive(BUFFER_SIZE, buf);

			// check destination ip address was found on network
			if ( plen!=0 )
			{
				// is reply my request?
				if (arp_packet_is_myreply_arp ( buf ) )
				{
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
				buf,
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
			// server response with ACK and SYN flag
			// three-way handshake
			plen = enc28j60PacketReceive(BUFFER_SIZE, buf);

			// if no new packet incoming, wait for it
			if ( plen == 0 )
				break;
			
			// else
			// check ip packet send to me or not?
			// accept ip packet only
			if (eth_type_is_ip_and_my_ip(buf, plen) == 0)
				break;
			
			// else
			// check SYNACK flag, after client send SYN server response by send SYNACK to client
			if (buf[TCP_FLAGS_P] == (TCP_FLAG_SYN_V | TCP_FLAG_ACK_V))
			{
				// client send ACK flag to open TCP session
				tcp_client_send_packet (
					buf,
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
				client_state = SEND_DATA;
			}
			syn_ack_timeout++;
			if (syn_ack_timeout > 3000) // 3s
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				client_state = IDLE;		// return to IDLE state
				client_data_ready =0;		// client data sent
			}
			break;
		case SEND_DATA:
			// mount data to buffer
			for (i = 0; i < data_length; i++)
			{
				buf[TCP_CHECKSUM_L_P+3+i] = databuf[i];
			}
				
	//		plen = fill_tcp_data(buf,0, "Tra Quang Kieu" );
			// send packet with PUSH-ACK
			tcp_client_send_packet (
				buf,
				dest_port,											 // destination port
				myport,								   // source port
				TCP_FLAG_ACK_V | TCP_FLAG_PUSH_V,						// flag
				0,											  // (bool)maximum segment size
				0,											  // (bool)clear sequence ack number
				0,											  // 0=use old seq, seqack : 1=new seq,seqack no data : >1 new seq,seqack with data
				data_length,						   // tcp data length
				dest_mac,
				dest_ip
			);
			
			syn_ack_timeout++;
			if (syn_ack_timeout > 3000) // 3s
			{
				// server not responding, fail to connect
				syn_ack_timeout = 0;
				client_state = IDLE;		// return to IDLE state
				client_data_ready =0;		// client data sent
			}
			
			// after client send data to server, server response by send ACK to client
			plen = enc28j60PacketReceive(BUFFER_SIZE, buf);

			// check destination ip address was found on network
			if ( plen!=0 )
			{
				// is ACK?
				if ( buf [ TCP_FLAGS_P ] == (TCP_FLAG_ACK_V) )
				{
					client_state = END_SESSION;
					syn_ack_timeout=0;
				}
			}
			
			break;
		case END_SESSION:
			// client send FIN to server
			tcp_client_send_packet (
				buf,
				dest_port,											 // destination port
				myport,								   // source port
				TCP_FLAG_FIN_V,				  // flag
				0,											  // (bool)maximum segment size
				0,											  // (bool)clear sequence ack number
				0,										   // 0=use old seq, seqack : 1=new seq,seqack no data : >1 new seq,seqack with data
				0,
				dest_mac,
				dest_ip
			);
			
			// server responding by send to client ACK
			plen = enc28j60PacketReceive(BUFFER_SIZE, buf);
			
			if ( plen!=0 )
			{
				// is ACK?
				if ( buf [ TCP_FLAGS_P ] == (TCP_FLAG_ACK_V) )
				{
					// send ACK with seqack = 1
					tcp_client_send_packet(
						buf,
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
			}
			
			break;
		default:
			syn_ack_timeout = 0;
			client_state = IDLE;		// return to IDLE state
			client_data_ready =0;		// client data sent
			break;
	}
}

void Client_sent(uint8_t * data, uint8_t length)
{
	int i;
	for (i = 0; i < length; i++)
		databuf[i] = data[i];
	data_length = length;
	client_data_ready = 1;
}
