#include "dhcp.h"

IP_ADDRESS DHCP_IP;
IP_ADDRESS BROADCAST_IP;
MAC_ADDRESS m_mac;

#define CLIENT_PORT 68
#define SERVER_PORT 67

// internal use only
#define TRUE 1
#define FALSE 0

volatile uint8_t DHCP_running = FALSE;

uint16_t m_checksum(uint8_t *buf, uint16_t len,uint8_t type);
void m_make_eth(uint8_t *buf);
void m_make_ip(uint8_t *buf);
void m_fill_ip_hdr_checksum(uint8_t *buf);
	
void DHCP_Discover(uint8_t * buff) 
{
	#ifdef DEBUG
	USART1_sendStr((char*) "DHCP: Discovery");
	#endif
	
	uint8_t i=0;
	int transaction_id = 123456;
    uint16_t ck;
	// add mac address
    m_make_eth(buff); // not clear
    // total length field in the IP header must be set:
    buff[IP_TOTLEN_H_P]=0;
    buff[IP_TOTLEN_L_P]=IP_HEADER_LEN+UDP_HEADER_LEN;
	// add ip address
    m_make_ip(buff);
	// server port: 67
    buff[UDP_DST_PORT_H_P]=SERVER_PORT>>8;
    buff[UDP_DST_PORT_L_P]=SERVER_PORT & 0xff;
	// client port: 68
    buff[UDP_SRC_PORT_H_P]=CLIENT_PORT>>8;
    buff[UDP_SRC_PORT_L_P]=CLIENT_PORT & 0xff;
    // calculte the udp length:
    buff[UDP_LEN_H_P]=0;
    buff[UDP_LEN_L_P]=UDP_HEADER_LEN;
    // zero the checksum
    buff[UDP_CHECKSUM_H_P]=0;
    buff[UDP_CHECKSUM_L_P]=0;
    ck=m_checksum(&buff[IP_SRC_P], 16,1);
    buff[UDP_CHECKSUM_H_P]=ck>>8;
    buff[UDP_CHECKSUM_L_P]=ck& 0xff;
	// add bootp message 
	buff[BOOTP_MSG_TYPE_P] = 0x01;
	buff[BOOTP_HARD_TYPE_P] = 0x01;
	buff[BOOTP_HARD_ADDR_LENGTH_P] = 0x06;
	buff[BOOTP_HARD_HOPS_P] = 0x00;
	buff[BOOTP_TRANSACTION_ID_P] = 		(transaction_id&0xff000000) > 24;
	buff[BOOTP_TRANSACTION_ID_P + 1] = 	(transaction_id&0xff0000) >> 16;
	buff[BOOTP_TRANSACTION_ID_P + 2] = 	(transaction_id&0xff00) >> 8;
	buff[BOOTP_TRANSACTION_ID_P + 3] = 	transaction_id&0xff;
	
	buff[BOOTP_SECONDS_ELAPSED_P] = 0x00;
	buff[BOOTP_SECONDS_ELAPSED_P+1] = 0x00;
	
	buff[BOOTP_BOOTP_FLAGS_P] = 0x00;
	buff[BOOTP_BOOTP_FLAGS_P+1] = 0x00;
	
	buff[BOOTP_CLIENT_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_CLIENT_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_CLIENT_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_CLIENT_IP_ADDRESS_P] = 0x00;
	
	buff[BOOTP_YOUR_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_YOUR_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_YOUR_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_YOUR_IP_ADDRESS_P] = 0x00;
	
	buff[BOOTP_NEXT_SERVER_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_NEXT_SERVER_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_NEXT_SERVER_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_NEXT_SERVER_IP_ADDRESS_P] = 0x00;
	
	buff[BOOTP_RELAY_AGENT_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_RELAY_AGENT_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_RELAY_AGENT_IP_ADDRESS_P] = 0x00;
	buff[BOOTP_RELAY_AGENT_IP_ADDRESS_P] = 0x00;
	
	buff[BOOTP_CLIENT_MAC_ADDRESS_P+0] = m_mac[0];
	buff[BOOTP_CLIENT_MAC_ADDRESS_P+1] = m_mac[1];
	buff[BOOTP_CLIENT_MAC_ADDRESS_P+2] = m_mac[2];
	buff[BOOTP_CLIENT_MAC_ADDRESS_P+3] = m_mac[3];
	buff[BOOTP_CLIENT_MAC_ADDRESS_P+4] = m_mac[4];
	buff[BOOTP_CLIENT_MAC_ADDRESS_P+5] = m_mac[5];
	
    enc28j60PacketSend(UDP_HEADER_LEN+IP_HEADER_LEN+ETH_HEADER_LEN,buff);
	
	#ifdef DEBUG
	USART1_sendByte(
	#endif
}	

void DHCP_Offer(uint8_t * buf) 
{
	#ifdef DEBUG
	USART1_sendStr((char*) "DHCP: Offer");
	#endif
}

void DHCP_Request(uint8_t * buf) 
{
	#ifdef DEBUG
	USART1_sendStr((char*) "DHCP: Request");
	#endif
}

void DHCP_Acknowledgement(uint8_t * buf) 
{
	#ifdef DEBUG
	USART1_sendStr((char*) "DHCP: Acknowledgement");
	#endif
}

void DHCP_Inform(uint8_t * buf, IP_ADDRESS clientIP) 
{
	#ifdef DEBUG
	USART1_sendStr((char*) "DHCP: Request");
	#endif
}
void DHCP_init(MAC_ADDRESS mac)
{
	// comment following line if you have right configuration
	#warning "Request Client initialized"
	
	// initial IP
	DHCP_IP[0] = 0;
	DHCP_IP[1] = 0;
	DHCP_IP[2] = 0;
	DHCP_IP[3] = 0;
	BROADCAST_IP[0] = 255;
	BROADCAST_IP[1] = 255;
	BROADCAST_IP[2] = 255;
	BROADCAST_IP[3] = 255;
	//copy mac address
	mac[0] = m_mac[0];
	mac[1] = m_mac[1];
	mac[2] = m_mac[2];
	mac[3] = m_mac[3];
	mac[4] = m_mac[4];
	mac[5] = m_mac[5];
}

void DHCP_getIP(uint8_t * buf, IP_ADDRESS ip, uint16_t timeout)
{
	DHCP_running = TRUE;
	DHCP_Discover(buf);
	DHCP_Offer(buf);
	DHCP_Request(buf);
	DHCP_Acknowledgement(buf);
	DHCP_running = FALSE;
}

extern uint8_t DHCP_informIP(uint8_t * buf)
{
	
	return TRUE;
}
	
uint16_t m_checksum(uint8_t *buf, uint16_t len,uint8_t type) {
    // type 0=ip 
    //      1=udp
    //      2=tcp
    uint32_t sum = 0;

    //if(type==0){
    //        // do not add anything
    //}
    if(type==1){
        sum+=IP_PROTO_UDP_V; // protocol udp
        // the length here is the length of udp (data+header len)
        // =length given to this function - (IP.scr+IP.dst length)
        sum+=len-8; // = real tcp len
    }
    if(type==2){
        sum+=IP_PROTO_TCP_V; 
        // the length here is the length of tcp (data+header len)
        // =length given to this function - (IP.scr+IP.dst length)
        sum+=len-8; // = real tcp len
    }
    // build the sum of 16bit words
    while(len >1){
        sum += 0xFFFF & (*buf<<8|*(buf+1));
        buf+=2;
        len-=2;
    }
    // if there is a byte left then add it (padded with zero)
    if (len){
        sum += (0xFF & *buf)<<8;
    }
    // now calculate the sum over the bytes in the sum
    // until the result is only 16bit long
    while (sum>>16){
        sum = (sum & 0xFFFF)+(sum >> 16);
    }
    // build 1's complement:
    return( (uint16_t) sum ^ 0xFFFF);
}
// make a return eth header from a received eth packet
void m_make_eth(uint8_t *buf)
{
    uint8_t i=0;
    //
    //copy the destination mac from the source and fill my mac into src
    while(i<6){
        buf[ETH_DST_MAC +i]=255; // broadcast mac address
        buf[ETH_SRC_MAC +i]=m_mac[i];
        i++;
    }
	buf[ETH_TYPE_H_P] = (ETHTYPE_IP_V & 0xff00) >> 8;
	buf[ETH_TYPE_L_P] = (ETHTYPE_IP_V & 0xff);
}

// make a return ip header from a received ip packet
void m_make_ip(uint8_t *buf)
{
    uint8_t i=0;
    while(i<4){
        buf[IP_DST_P+i]=BROADCAST_IP[i];
        buf[IP_SRC_P+i]=DHCP_IP[i];
        i++;
    }
    m_fill_ip_hdr_checksum(buf);
}

void m_fill_ip_hdr_checksum(uint8_t *buf)
{
    uint16_t ck;
    // clear the 2 byte checksum
    buf[IP_CHECKSUM_P]=0;
    buf[IP_CHECKSUM_P+1]=0;
    buf[IP_FLAGS_P]=0x40; // don't fragment
    buf[IP_FLAGS_P+1]=0;  // fragement offset
    buf[IP_TTL_P]=64; // ttl
    // calculate the checksum:
    ck=m_checksum(&buf[IP_P], IP_HEADER_LEN,0);
    buf[IP_CHECKSUM_P]=ck>>8;
    buf[IP_CHECKSUM_P+1]=ck& 0xff;
}

