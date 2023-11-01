#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
//#include "key.h"
//#include "lcd.h"
//#include "usmart.h"  
//#include "usart2.h"  
#include "msp8266.h"  //msp8266的通信函数


#include "timer.h" 
#include "ov2640.h" 
#include "dcmi.h" 


#include "M8266HostIf.h"
#include "M8266WIFIDrv.h"
#include "M8266WIFI_ops.h"
#include "brd_cfg.h"

//2.20开发进度 去掉LCD和按键的代码，由于PCB板子上没有这部分的电路，同时保留原有的usart3，便于以后进行串口通信
//设置为tcp通信，并摄像头作为tcp的客户端，设计相关函数分为握手，数据传输的环节


//开发建议：建议照片缓存数据一定在主函数，之后其他函数调用相关数列的地址
int main(void)
{ 
		u8 success=0;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  //初始化延时函数
//	uart_init(115200);		//初始化串口波特率为115200
	//usart2_init(42,115200);		//初始化串口2波特率为115200
	LED_Init();					//初始化LED 
 
	while(OV2640_Init())//初始化OV2640
	{
		LED_set(1, 0); 
		LED_set(1, 1); 	
		delay_ms(200);
	}
	M8266HostIf_Init();
		success = M8266WIFI_Module_Init_Via_SPI();
  if(success)
	{
			M8266WIFI_Module_delay_ms(100);		
	}
	else // If M8266WIFI module initialisation failed, two led constantly flash in 2Hz
	{
		while(1)   //出现错误
		{
		LED_set(1, 1);  
		LED_set(0, 0); 
		delay_ms(600);
		LED_set(1, 0); 
		LED_set(0, 1); 	
		delay_ms(600);
		}
	}	
Msp8266_Receive_exit_Init();//按键中断的初始化
	if(ov2640_mode)
	{
		if (Msp8266_Init())
		{	
	   delay_ms(50);
		}

	}
	else
	{
		while(1)
		{
		LED_set(1, 1);  
		delay_ms(200);
		LED_set(0, 0); 
    delay_ms(200);			
		}
	}
while(1)
{
		LED_set(0, 1);  
		delay_ms(400);
		LED_set(0, 0); 
    delay_ms(400);	
}
}
