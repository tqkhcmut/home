/** 
* this is main program
**/
#include "stm32f10x.h"
#include "assistant.h"
#include "converter.h"
#include "delay/delay.h"
#include "usart/usart.h"
#include "ENC28J60/client.h"
#include "Utilities/STM32vldiscovery.h"
#include <string.h>


int main()
{	
	IP_ADDRESS Server_ip = {192, 168, 1, 101};
	int Server_port = 1412;
	IP_ADDRESS Client_ip = {192, 168, 1, 123};
	int Client_port = 1311;
	
	int tmpTime = millis();
	char buffer[12];
	int fetched = 0;
	
	/* System clocks configuration ---------------------------------------------*/
	RCC_Configuration();

	/* GPIO configuration ------------------------------------------------------*/
	GPIO_Configuration();

	/* NVIC configuration ------------------------------------------------------*/
	NVIC_Configuration();
	
	// delay
	delay_init();
	
	// USART init
	USART1_init();
	
	// init led + button
	STM32vldiscovery_LEDInit(LED3);
	STM32vldiscovery_LEDInit(LED4);
	STM32vldiscovery_PBInit(BUTTON_USER, BUTTON_MODE_GPIO);
	
	// initial client
	Client_init(Client_ip, Client_port);
	Client_connect(Server_ip, Server_port);
	
	Client_sendStr((char*)"Ping\r\n"); // check the connection
	
	USART1_sendStr((char*)"Tra Quang Kieu");
		
	while (1)
	{
		if (millis() - tmpTime > 1000)
		{
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
			
//			if (STM32vldiscovery_PBGetState(BUTTON_USER) != 0)
//			{
//				Client_sendStr((char*)"Switch on\r\n");
//			}
//			else 
//			{
//				Client_sendStr((char*)"Switch off\r\n");
//			}
			
			USART1_sendNum(fetched);
			USART1_sendStr((char*)buffer);
			tmpTime = millis();
		}
	}
}

