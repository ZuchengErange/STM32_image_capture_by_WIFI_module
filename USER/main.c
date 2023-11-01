#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
//#include "key.h"
//#include "lcd.h"
//#include "usmart.h"  
//#include "usart2.h"  
#include "msp8266.h"  //msp8266��ͨ�ź���


#include "timer.h" 
#include "ov2640.h" 
#include "dcmi.h" 


#include "M8266HostIf.h"
#include "M8266WIFIDrv.h"
#include "M8266WIFI_ops.h"
#include "brd_cfg.h"

//2.20�������� ȥ��LCD�Ͱ����Ĵ��룬����PCB������û���ⲿ�ֵĵ�·��ͬʱ����ԭ�е�usart3�������Ժ���д���ͨ��
//����Ϊtcpͨ�ţ�������ͷ��Ϊtcp�Ŀͻ��ˣ������غ�����Ϊ���֣����ݴ���Ļ���


//�������飺������Ƭ��������һ������������֮��������������������еĵ�ַ
int main(void)
{ 
		u8 success=0;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
	delay_init(168);  //��ʼ����ʱ����
//	uart_init(115200);		//��ʼ�����ڲ�����Ϊ115200
	//usart2_init(42,115200);		//��ʼ������2������Ϊ115200
	LED_Init();					//��ʼ��LED 
 
	while(OV2640_Init())//��ʼ��OV2640
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
		while(1)   //���ִ���
		{
		LED_set(1, 1);  
		LED_set(0, 0); 
		delay_ms(600);
		LED_set(1, 0); 
		LED_set(0, 1); 	
		delay_ms(600);
		}
	}	
Msp8266_Receive_exit_Init();//�����жϵĳ�ʼ��
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
