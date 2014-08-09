/** 
* this is main program
**/
#include "assistant.h"
#include "converter.h"
#include "ENC28J60/client.h"
#include "ENC28J60/dhcp.h"
#include "Utilities/STM32vldiscovery.h"
#include <string.h>


int main()
{	
	IP_ADDRESS Server_ip = {192, 168, 1, 101};
	uint16_t Server_port = 1412;
	IP_ADDRESS Client_ip = {192, 168, 1, 123};
	uint16_t Client_port = 1311;
	
	/* System clocks configuration ---------------------------------------------*/
	RCC_Configuration();

	/* GPIO configuration ------------------------------------------------------*/
	GPIO_Configuration();

	/* NVIC configuration ------------------------------------------------------*/
	NVIC_Configuration();
	
	// USART init
	USART1_init();
	
	// delay
	delay_init();
	
	// init led
	STM32vldiscovery_LEDInit(LED3);
	STM32vldiscovery_LEDInit(LED4);
	STM32vldiscovery_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	
	Client_init(Client_ip, Client_port);
	Client_connect(Server_ip, Server_port);
	
	Client_sendStr((char*)"Ping\r\n"); // check the connection
	
	USART1_sendStr((char*)"Tra Quang Kieu");
	
	int tmpTime = millis();
	uint8_t buffer[12];
	int fetched = 0;
		
	while (1)
	{
		if (millis() - tmpTime > 500)
		{
//			STM32vldiscovery_LEDToggle(LED3);
//			USART1_sendStr((char*)"Ping\n");
//			Client_sendStr((char*)"Ping ");
//			Client_sendStr(Num_toString(tmpTime));
//			Client_sendStr((char*)"\r\n");
//			Client_makeFinal();
//			Client_getIP();
			for (int i = 0; i < 12; i++)
				buffer[i] = 0;
			fetched = Client_read(buffer, 10);
			if (strcmp((char*)buffer, "LED3 On\r\n") == 0)
			{
				STM32vldiscovery_LEDOn(LED3);
			}
			else if (strcmp((char*)buffer, "LED3 Off\r\n") == 0)
			{
				STM32vldiscovery_LEDOff(LED3);
			}
			else if (strcmp((char*)buffer, "LED4 On\r\n") == 0)
			{
				STM32vldiscovery_LEDOn(LED4);
			}
			else if (strcmp((char*)buffer, "LED4 Off\r\n") == 0)
			{
				STM32vldiscovery_LEDOff(LED4);
			}
			
			if (STM32vldiscovery_PBGetState(BUTTON_USER) != 0)
			{
				Client_sendStr((char*)"Switch on\r\n");
			}
			else 
			{
				Client_sendStr((char*)"Switch off\r\n");
			}
			
			USART1_sendNum(fetched);
			USART1_sendStr((char*)buffer);
			tmpTime = millis();
//			Client_makeFinal();
		}
//		smallDelay();
	}
}

