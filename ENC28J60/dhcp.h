#ifndef dhcp_h
#define dhcp_h

#ifdef __cplusplus
extern "C" {
#endif
	
	#include <inttypes.h>
	#include "client.h"
	#include "net.h"
	#include "enc28j60.h"
	
	extern volatile uint8_t DHCP_running;
	
	extern void DHCP_init(MAC_ADDRESS mac);
	extern void DHCP_getIP(uint8_t * buf, IP_ADDRESS ip, uint16_t timeout);
	extern uint8_t DHCP_informIP(uint8_t * buf);

#ifdef __cplusplus
}
#endif

#endif
