#ifndef __LED_H
#define __LED_H
#include "sys.h"

//LED�˿ڶ���
#define LED0 PEout(2)	// DS0
#define LED1 PEout(3)	// DS1	 

void LED_Init(void);//��ʼ��	

void LED_set(u8 led_no, u8 on);

#endif
