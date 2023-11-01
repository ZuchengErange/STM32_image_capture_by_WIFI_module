#include "brd_cfg.h"
#include "led.h"  

void LED_set(u8 led_no, u8 on)
{
	if(led_no==0)
	{
		if(on)
		   GPIO_ResetBits(LED_GPIOS, LED0_GPIO_PIN);
		else
			 GPIO_SetBits(LED_GPIOS, LED0_GPIO_PIN);
	}
	else
	{
		if(on)
		   GPIO_ResetBits(LED_GPIOS, LED1_GPIO_PIN);
		else
			 GPIO_SetBits(LED_GPIOS, LED1_GPIO_PIN);
	}
}


void LED_Init(void)
{    	 
  GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIO_LEDS, ENABLE);		// Enable the GPIOx Clock for LED GPIO

	GPIO_InitStructure_AS_GPIO_OUTPUT(LED_GPIOS,LED0_GPIO_PIN|LED1_GPIO_PIN);
		
	LED_set(0, 0);  // light off the LED initially
	LED_set(1, 0);
}




