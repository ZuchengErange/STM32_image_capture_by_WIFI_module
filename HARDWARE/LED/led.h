#ifndef __LED_H
#define __LED_H
#include "sys.h"

//LED端口定义
#define LED0 PEout(2)	// DS0
#define LED1 PEout(3)	// DS1	 

void LED_Init(void);//初始化	

void LED_set(u8 led_no, u8 on);

#endif
