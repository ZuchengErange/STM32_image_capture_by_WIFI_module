#include "stdio.h"
#include "delay.h"
#include "brd_cfg.h"
#include "M8266WIFIDrv.h"
#include "M8266HostIf.h"
#include "M8266WIFI_ops.h"
#include "led.h"
#include "msp8266.h"  

#include "ov2640.h"    //����ͷ��
#include "dcmi.h" 


//����ͨ�ŷ�ʽΪTCP�ͻ���  ������ΪTCP���������в���  ����ͷ��ΪTCP�ͻ���

//local port	(Chinese���׽��ֵı��ض˿�)
#define TEST_CONNECTION_TYPE   1	   //����ΪTCP�ͻ���
#define TEST_LOCAL_PORT  			0			//           local port=0 will generate a random local port each time fo connection. To avoid the rejection by TCP server due to repeative connection with the same ip:port
    // (Chinese: ��local port���ݵĲ���Ϊ0ʱ�����ض˿ڻ������������һ�����ģ�����ͻ��˷������ӷ�����ʱ�����á���Ϊ��������Ķ˿�ÿ�λ᲻һ�����Ӷ������������β���ͬ���ĵ�ַ�Ͷ˿�����ʱ���������ܾ���
                                // (Chinese: ���ģ����ΪTCP��������UDP����ô����ָ�����ض˿�
   //// if module as TCP Client (Chinese:���ģ����ΪTCP�ͻ��ˣ���Ȼ����ָ��Ŀ���ַ��Ŀ��˿ڣ���ģ����Ҫȥ���ӵ�TCP�������ĵ�ַ�Ͷ˿�)
#define TEST_REMOTE_ADDR    	 	"192.168.43.211"  //��Ϊ �Լҷ�����"192.168.3.7"   ��ѧУ��Ϊ192.168.43.211  �ֻ��ȵ�
#define TEST_REMOTE_PORT  	    2214						// 80  ����Ϊ2214  "192.168.3.11" 

#define  RECV_DATA_MAX_SIZE  256    //������հ������ݳ��� 	
	 u8  RecvData[RECV_DATA_MAX_SIZE];    //Ҫ�����У�飬ȷ����������
   u16 received = 0;  //��ʾ�յ������ݳ���
   u8 Search_flag=0;  //��ѯ��־λ

#define jpeg_buf_size 16*1024  			//����JPEG���ݻ���jpeg_buf�Ĵ�С(*4�ֽ�)
__align(4) u32 jpeg_buf[jpeg_buf_size];	//JPEG���ݻ���buf
volatile u32 jpeg_data_len=0; 			//buf�е�JPEG��Ч���ݳ��� 
volatile u8 jpeg_data_ok=0;				//JPEG���ݲɼ���ɱ�־ 
										//0,����û�вɼ���;
										//1,���ݲɼ�����,���ǻ�û����;
										//2,�����Ѿ����������,���Կ�ʼ��һ֡����

u32 Start_data=0xFEDCBA98;  // �����������
u16 status = 0;    //����״̬����

//JPEG�ߴ�֧���б�
const u16 jpeg_img_size_tbl[][2]=
{
	176,144,	//QCIF
	160,120,	//QQVGA
	352,288,	//CIF
	320,240,	//QVGA
	640,480,	//VGA
	800,600,	//SVGA
	1024,768,	//XGA
	1280,1024,	//SXGA
	1600,1200,	//UXGA
}; 

u8 Error_ODER[]="ERROR ORDER!";   //ָ��id���ݣ����ɸ���
u8 Picture[]="PICTURE";
u8 Video[]="VIDEO";
u8 Reset[]="RESET";
u8 UPC[]="UPC+";
u8 Device[]="DEVICE";
u8 Wifi_test[]="WIFI_TEST";
u8 start[]="start";
u8 end[]="end";
u8 stop[]="STOP";
u8 device_status_check[]="Status_check";
u8 packet_loss_check[]="packet_loss_check";
u8 size_small[]="SIZE4";
u8 size_medium[]="SIZE6";
u8 size_big[]="SIZE8";
u8 shake_hands[]="SHAKE";

u8 device_id[]="UPC_video_01";

u8 pic_size=5;	  //����ͼƬ�ߴ��С
void Msp8266_start(void)
{

	u8 len=sizeof(start);
//	u8 len1=sizeof(*Msp8266_oder[1]);  �������
	
	M8266WIFI_SPI_Send_BlockData(start,len, 5000, 0, NULL, 0, &status);
	
}

void Msp8266_end(void)
{

	u8 len=sizeof(end);
//	u8 len1=sizeof(*Msp8266_oder[1]);  �������
	
	M8266WIFI_SPI_Send_BlockData(end,len, 5000, 0, NULL, 0, &status);
	
}


//����JPEG����
//���ɼ���һ֡JPEG���ݺ�,���ô˺���,�л�JPEG BUF.��ʼ��һ֡�ɼ�.
void jpeg_data_process(void)
{
	if(ov2640_mode)//ֻ����JPEG��ʽ��,����Ҫ������.
	{
		if(jpeg_data_ok==0)	//jpeg���ݻ�δ�ɼ���
		{	
			DMA_Cmd(DMA2_Stream1, DISABLE);//ֹͣ��ǰ���� 
			while (DMA_GetCmdStatus(DMA2_Stream1) != DISABLE){}//�ȴ�DMA2_Stream1������  
			jpeg_data_len=jpeg_buf_size-DMA_GetCurrDataCounter(DMA2_Stream1);//�õ��˴����ݴ���ĳ���
				
			jpeg_data_ok=1; 				//���JPEG���ݲɼ��갴��,�ȴ�������������
		}
		if(jpeg_data_ok==2)	//��һ�ε�jpeg�����Ѿ���������
		{
			DMA2_Stream1->NDTR=jpeg_buf_size;	
			DMA_SetCurrDataCounter(DMA2_Stream1,jpeg_buf_size);//���䳤��Ϊjpeg_buf_size*4�ֽ�
			DMA_Cmd(DMA2_Stream1, ENABLE);			//���´���
			jpeg_data_ok=0;						//�������δ�ɼ�
		}
	}
} 

//JPEG����
//JPEG����,ͨ��wifi���͸�����.
void jpeg_test(void)
{
	u8 *p;
//	u8 key;
//	u8 effect=0,saturation=2,contrast=2;
//	size=5;		//Ĭ����QVGA 320*240�ߴ�
 
	jpeg_data_ok=0;   //��ʼ������һ�ν�������

	
 	OV2640_JPEG_Mode();		//JPEGģʽ
	My_DCMI_Init();			//DCMI����
	DCMI_DMA_Init((u32)&jpeg_buf,jpeg_buf_size,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA����   
	OV2640_OutSize_Set(jpeg_img_size_tbl[pic_size][0],jpeg_img_size_tbl[pic_size][1]);//��������ߴ� 
	DCMI_Start(); 		//��������
	p=(u8*)jpeg_buf;  //pȡjpeg_buf�ĵ�ַ
	while(1)
	{
		delay_us(100);
		LED1=!LED1;	
	if(jpeg_data_ok==1)
	{		
      M8266WIFI_SPI_Send_BlockData(p, (u32)jpeg_data_len*4, 40, 0, NULL, 0, &status);
		jpeg_data_ok=3;    //����Ϊ3�����������жϣ����Ҳ��ٶ����ݽ��д���
    DCMI_ITConfig(DCMI_IT_FRAME,DISABLE);//�ر�֡�ж� 
    DCMI_Stop();		//�رմ���
		break;
	}
  
	}    
}

void jpeg_picture(void)
{
	u8 *p;
//	size=5;		//Ĭ����QVGA 320*240�ߴ� 
  jpeg_data_ok=0;   //��ʼ������һ�ν�������
	
 	OV2640_JPEG_Mode();		//JPEGģʽ
	My_DCMI_Init();			//DCMI����
	DCMI_DMA_Init((u32)&jpeg_buf,jpeg_buf_size,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA����   
	OV2640_OutSize_Set(jpeg_img_size_tbl[pic_size][0],jpeg_img_size_tbl[pic_size][1]);//��������ߴ� 
	DCMI_Start(); 		//��������
	p=(u8*)jpeg_buf;
	while(1)
	{
		delay_us(100);
		LED1=!LED1;	
	if(jpeg_data_ok==1)
	{		
		  Msp8266_start();  //��ʼУ�� ����tcpУ��
		
      M8266WIFI_SPI_Send_BlockData(p, (u32)jpeg_data_len*4, 500, 0, NULL, 0, &status);
		  delay_us(100);
			Msp8266_end();   //����У�飬����tcp��������
		
		//	 Communication Test (Chinese: WIFI�׽��ֵ������շ�ͨ�Ų���)
		jpeg_data_ok=3;    //����Ϊ3�����������жϣ����Ҳ��ٶ����ݽ��д���
//    DCMI_ITConfig(DCMI_IT_FRAME,DISABLE);//�ر�֡�ж� 
//    DCMI_Stop();		//�رմ���
		break;
	}
	} 	
}
void jpeg_video(void)  //������Ƶ����
{
	u8 *p;
//	size=5;		//Ĭ����QVGA 320*240�ߴ� 
	jpeg_data_ok=0;   //��ʼ������һ�ν�������
	
	
 	OV2640_JPEG_Mode();		//JPEGģʽ
	My_DCMI_Init();			//DCMI����
	DCMI_DMA_Init((u32)&jpeg_buf,jpeg_buf_size,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA����   
	OV2640_OutSize_Set(jpeg_img_size_tbl[pic_size][0],jpeg_img_size_tbl[pic_size][1]);//��������ߴ� 
	DCMI_Start(); 		//��������
	
	p=(u8*)jpeg_buf;

	
	while(1)
	{
		delay_us(100);
		LED1=!LED1;	
	if(jpeg_data_ok==1)
	{		
		
      M8266WIFI_SPI_Send_BlockData(p, (u32)jpeg_data_len*4, 5000, 0, NULL, 0, &status);
		  delay_us(100);

		
		jpeg_data_ok=2;	//���jpeg���ݴ�������,������DMAȥ�ɼ���һ֡��.
	if (M8266WIFI_SPI_Has_DataReceived()==1) //���wifi�յ�������
	{
		jpeg_data_ok=3;    //����Ϊ3�����������жϣ����Ҳ��ٶ����ݽ��д���
//    DCMI_ITConfig(DCMI_IT_FRAME,DISABLE);//�ر�֡�ж� 
//    DCMI_Stop();		//�رմ���
		break;
	}
	}
	} 	
}
//���Ͳ���������֤������   ���Թ����г��ַ�������Ϊ0 ,���ڿ��Ƿ���һ��2048�ֽڵ����ݣ��������ض����ɣ��ԱȽ��յ��ֽ�����ȷ��������
void Msp8266_packet_loss_detect(void)
{
	u8 *p;
	int i;
	u8 snd_data[256]; //���һ��256���ȵ����飬���ݷ�Ϊ4�η��ͣ����Ͷ���0-255��֮��У��ÿ�ν��ճ̶�,�󶪰���ƽ��ֵ
	p=(u8*)snd_data;
	for(i=0;i<256;i++)
		snd_data[i]=i;

	M8266WIFI_SPI_Send_BlockData(p,sizeof(snd_data), 5000, 0, NULL, 0, &status);
	delay_us(100);
}
//�ظ������ʾ�յ�������ʼִ������
void Msp8266_send_ok(void)
{
  u8 ok[]="ok!";
	u8 len=sizeof(ok);
//	u8 len1=sizeof(*Msp8266_oder[1]);  �������
	
	M8266WIFI_SPI_Send_BlockData(ok,len, 5000, 0, NULL, 0, &status);
}


void Msp8266_Initial_verification(void)  //���ݳ�ʼAT+У��
{
	
}

u8 string_check(u8 order[],u8 Initial[],u8 len) //orderΪ�������� InitialΪУ������ lenΪ�ַ�������
{
	u8 i;
	for(i=0;i<len;i++)
	{
	if(order[i]!=Initial[i])	
		return 0;
	}

	return 1;
}
//ָ���ѯ��ִ��
void Msp8266_oder_inquire_do(u16 rec,u8* rec_data)  //recΪ���ݳ��� rec_dataΪʵ������
{
	u8 i=0,status_check=0;
	u8 Initial_data[3]="AT+";   //��������������
	u8 buf[15];  //�����洢ȥ��at+������ָ��

	for(i=0;i<=2;i++)  //У��ǰ��λ��
	{
		if ((Initial_data[i]!=rec_data[i])||(rec-3<=0))
		{
			status_check=1; //��ʾϵͳУ�鲻ͨ��
			break;
		}
	}
	if(status_check==1)  //У�鲻ͨ��ʱ����error oder��
	{
  delay_us(80);
		
  M8266WIFI_SPI_Send_BlockData(Error_ODER,sizeof(Error_ODER), 5000, 0, NULL, 0, &status); //���ݷ��أ������յ�������	
//3.8 ����һ����������⣬ͬ����������Ҫ�෢һ��Ż���Ч������ �о����ж�����̫�ණ��

		//���ǵ�����޷�����ַ�������һ���������ô�Ϳ���ָ��У��������
		//�����ѽ������debug���û��������
	//3.8�޸���� �����޷�����ָ��ķ�ʽ�������ݣ����Կ���ֻд����ָ��			
	}
	else
	{
	for(i=3;i<rec;i++)  //У��ͨ���Ժ�洢������������
		buf[i-3]=rec_data[i];
		if (string_check(buf,Picture,7))  //�Ƚ��Ƿ�Ϊ�������� picture�ַ�������Ϊ7
		{
			 delay_us(20);
//		Msp8266_start_send();  //���ݴ������
	   delay_ms(50);
		jpeg_picture();  //��һ����			
		}	
		else if(string_check(buf,Reset,5))     //�ַ�������Ϊ5
		{
		__set_FAULTMASK(1); // ��λ����
    NVIC_SystemReset(); // 
		}
		else if(string_check(buf,Video,5))   //�ַ�������Ϊ5
		{
			jpeg_video();
		}
		else if(string_check(buf,stop,4))
		{
		 delay_ms(10);  //ʲô���������ص��Ⱥ�ָ���״̬	
		}
		else if(string_check(buf,device_status_check,12)) //ϵͳ״̬��⣬չʾϵͳ����״̬��ϵͳ����ʱ�����ϵͳ�����ʲ���
		{
//			u8 tem[]="Device id is  ";
			u8 tem1[]="ov2640 is ready\n";		
			u8 len;
//			u8 len=sizeof(tem);
//			u8 len1;
//			M8266WIFI_SPI_Send_BlockData(tem, len, 5000, 0, NULL, 0, &status); //����ǰ��
//			delay_us(20);
//			len1=sizeof(device_id);
//			M8266WIFI_SPI_Send_BlockData(device_id, len1, 5000, 0, NULL, 0, &status); //����ϵͳid
//			delay_us(20);
      while(OV2640_Init())
			{
		  LED_set(1, 0); 
		  LED_set(1, 1); 	
		  delay_ms(200);
			}		
      len=sizeof(tem1);
      M8266WIFI_SPI_Send_BlockData(tem1, len, 5000, 0, NULL, 0, &status); //����ov2640׼������		
      delay_us(20);
//      Msp8266_packet_loss_detect();			//ϵͳ�����ʼ��  //���ǵ������ʼ���Ϊ���ӡ�����ʱ����һ����ȥд4.29		
		}
		else if(string_check(buf,packet_loss_check,17)) //ϵͳ�����ʼ�⣬��ϸ˵��������
		{
			Msp8266_packet_loss_detect();
		}
		else if(string_check(buf,size_small,5))   //���ͼƬ�ߴ��С
		{
		  pic_size=4;	
		}
		else if(string_check(buf,size_medium,5))//��
		{
			pic_size=6;
		}
		else if(string_check(buf,size_big,5))//��
		{
			pic_size=8;
		}
		else if(string_check(buf,shake_hands,5)) //�����ź��ж�
		{
		  Msp8266_send_ok();
		}
		else if(string_check(buf,Device,6))
		{
			u8 len2=sizeof(device_id);
			M8266WIFI_SPI_Send_BlockData(device_id, len2, 5000, 0, NULL, 0, &status); //����ϵͳid
		}	
	}
}


// ����MSP8266Ϊ�ͻ���TCP 
u8 Msp8266_Init(void)
{
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	 Setup Connection and Config connection upon neccessary (Chinese: �����׽��֣��Լ���Ҫʱ���׽��ֵ�һЩ����)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	 
	 u16 status = 0;
	 u8  link_no=0;
//step 0: config tcp windows number (Chinese: ����0�������TCP���͵��׽��֣��������õ������ڲ�����

// u8 M8266WIFI_SPI_Config_Tcp_Window_num(u8 link_no, u8 tcp_wnd_num, u16* status)

 if(M8266WIFI_SPI_Config_Tcp_Window_num(link_no, 4, &status)==0)
  {
     LED0=1;
		 delay_ms(100);
		 return 0;
	}
	LED0=0;

//step 1: setup connection (Chinese: ����1�������׽������ӣ�
  //  u8 M8266WIFI_SPI_Setup_Connection(u8 tcp_udp, u16 local_port, char remote_addr, u16 remote_port, u8 link_no, u8 timeout_in_s, u16* status);
	if(M8266WIFI_SPI_Setup_Connection(TEST_CONNECTION_TYPE, TEST_LOCAL_PORT, TEST_REMOTE_ADDR, TEST_REMOTE_PORT, link_no, 20, &status)==0)
	{		
     LED1=1;
		 delay_ms(100);
		 return 0;
	}
	LED1=0;
	      // setup connection successfully, we could config it (Chinese: �����׽��ֳɹ����Ϳ��������׽��֣�
 return 1;  //��ʾ�ɹ�
}
//�����жϳ�ʼ��
void Msp8266_Receive_exit_Init()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;
	EXTI_InitTypeDef   EXTI_InitStructure;	

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹ��GPIOAʱ��
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; //wifi���ⲿ�ж�
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//��ͨ����ģʽ
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;//����
  GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��GPIOA3   GPIO��ʼ��
	
		 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//ʹ��SYSCFGʱ��
	
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource3);//PA2 ���ӵ��ж���3
	
	/* ����EXTI_Line2 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line3 ;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//�ж��¼�
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //�����ش���
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;//�ж���ʹ��
  EXTI_Init(&EXTI_InitStructure);//����
 
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;//�ⲿ�ж�3
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;//��ռ���ȼ�1
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;//�����ȼ�2
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
  NVIC_Init(&NVIC_InitStructure);//���� 
}

void EXTI3_IRQHandler(void)   //�����ж� ϵͳ�������գ������жϺ�ִ��  
{

	u8  link_no=0;	
   LED0=!LED0;  
	if (M8266WIFI_SPI_Has_DataReceived()==1) //���wifi�յ�������
	{
	 received=M8266WIFI_SPI_RecvData(RecvData, RECV_DATA_MAX_SIZE,2000,0,&status); 

//		Msp8266_send_ok(); //����ok��ָ���ʾ�����յ�
		
		Search_flag=1;
		
		
		
	Msp8266_oder_inquire_do(received,RecvData);
		
		
//	 M8266WIFI_SPI_Send_BlockData(RecvData,received, 5000, 0, NULL, 0, &status); //���ݷ��أ������յ�������
		
		//3.8 ���ڵ����⣬ͬ����������Ҫ�ٷ�һ��Ż���Ч������ 
		
		//�����ѽ��������Ϊdebug���ûת��
	 EXTI_ClearITPendingBit(EXTI_Line2);//���LINE2�ϵ��жϱ�־λ
	}		
}	


