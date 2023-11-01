#include "stdio.h"
#include "delay.h"
#include "brd_cfg.h"
#include "M8266WIFIDrv.h"
#include "M8266HostIf.h"
#include "M8266WIFI_ops.h"
#include "led.h"
#include "msp8266.h"  

#include "ov2640.h"    //摄像头端
#include "dcmi.h" 


//设置通信方式为TCP客户端  电脑作为TCP服务器进行测试  摄像头作为TCP客户端

//local port	(Chinese：套接字的本地端口)
#define TEST_CONNECTION_TYPE   1	   //设置为TCP客户端
#define TEST_LOCAL_PORT  			0			//           local port=0 will generate a random local port each time fo connection. To avoid the rejection by TCP server due to repeative connection with the same ip:port
    // (Chinese: 当local port传递的参数为0时，本地端口会随机产生。这一点对于模组做客户端反复连接服务器时很有用。因为随机产生的端口每次会不一样，从而避免连续两次采用同样的地址和端口链接时被服务器拒绝。
                                // (Chinese: 如果模组作为TCP服务器或UDP，那么必须指定本地端口
   //// if module as TCP Client (Chinese:如果模组作为TCP客户端，当然必须指定目标地址和目标端口，即模组所要去连接的TCP服务器的地址和端口)
#define TEST_REMOTE_ADDR    	 	"192.168.43.211"  //改为 自家服务器"192.168.3.7"   在学校改为192.168.43.211  手机热点
#define TEST_REMOTE_PORT  	    2214						// 80  设置为2214  "192.168.3.11" 

#define  RECV_DATA_MAX_SIZE  256    //定义接收包的数据长度 	
	 u8  RecvData[RECV_DATA_MAX_SIZE];    //要求进行校验，确定数据无误
   u16 received = 0;  //表示收到的数据长度
   u8 Search_flag=0;  //查询标志位

#define jpeg_buf_size 16*1024  			//定义JPEG数据缓存jpeg_buf的大小(*4字节)
__align(4) u32 jpeg_buf[jpeg_buf_size];	//JPEG数据缓存buf
volatile u32 jpeg_data_len=0; 			//buf中的JPEG有效数据长度 
volatile u8 jpeg_data_ok=0;				//JPEG数据采集完成标志 
										//0,数据没有采集完;
										//1,数据采集完了,但是还没处理;
										//2,数据已经处理完成了,可以开始下一帧接收

u32 Start_data=0xFEDCBA98;  // 定义测试数据
u16 status = 0;    //定义状态数据

//JPEG尺寸支持列表
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

u8 Error_ODER[]="ERROR ORDER!";   //指令id数据，不可更改
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

u8 pic_size=5;	  //设置图片尺寸大小
void Msp8266_start(void)
{

	u8 len=sizeof(start);
//	u8 len1=sizeof(*Msp8266_oder[1]);  情况测试
	
	M8266WIFI_SPI_Send_BlockData(start,len, 5000, 0, NULL, 0, &status);
	
}

void Msp8266_end(void)
{

	u8 len=sizeof(end);
//	u8 len1=sizeof(*Msp8266_oder[1]);  情况测试
	
	M8266WIFI_SPI_Send_BlockData(end,len, 5000, 0, NULL, 0, &status);
	
}


//处理JPEG数据
//当采集完一帧JPEG数据后,调用此函数,切换JPEG BUF.开始下一帧采集.
void jpeg_data_process(void)
{
	if(ov2640_mode)//只有在JPEG格式下,才需要做处理.
	{
		if(jpeg_data_ok==0)	//jpeg数据还未采集完
		{	
			DMA_Cmd(DMA2_Stream1, DISABLE);//停止当前传输 
			while (DMA_GetCmdStatus(DMA2_Stream1) != DISABLE){}//等待DMA2_Stream1可配置  
			jpeg_data_len=jpeg_buf_size-DMA_GetCurrDataCounter(DMA2_Stream1);//得到此次数据传输的长度
				
			jpeg_data_ok=1; 				//标记JPEG数据采集完按成,等待其他函数处理
		}
		if(jpeg_data_ok==2)	//上一次的jpeg数据已经被处理了
		{
			DMA2_Stream1->NDTR=jpeg_buf_size;	
			DMA_SetCurrDataCounter(DMA2_Stream1,jpeg_buf_size);//传输长度为jpeg_buf_size*4字节
			DMA_Cmd(DMA2_Stream1, ENABLE);			//重新传输
			jpeg_data_ok=0;						//标记数据未采集
		}
	}
} 

//JPEG测试
//JPEG数据,通过wifi发送给电脑.
void jpeg_test(void)
{
	u8 *p;
//	u8 key;
//	u8 effect=0,saturation=2,contrast=2;
//	size=5;		//默认是QVGA 320*240尺寸
 
	jpeg_data_ok=0;   //初始化，再一次进行拍照

	
 	OV2640_JPEG_Mode();		//JPEG模式
	My_DCMI_Init();			//DCMI配置
	DCMI_DMA_Init((u32)&jpeg_buf,jpeg_buf_size,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA配置   
	OV2640_OutSize_Set(jpeg_img_size_tbl[pic_size][0],jpeg_img_size_tbl[pic_size][1]);//设置输出尺寸 
	DCMI_Start(); 		//启动传输
	p=(u8*)jpeg_buf;  //p取jpeg_buf的地址
	while(1)
	{
		delay_us(100);
		LED1=!LED1;	
	if(jpeg_data_ok==1)
	{		
      M8266WIFI_SPI_Send_BlockData(p, (u32)jpeg_data_len*4, 40, 0, NULL, 0, &status);
		jpeg_data_ok=3;    //设置为3，不再启用中断，并且不再对数据进行处理
    DCMI_ITConfig(DCMI_IT_FRAME,DISABLE);//关闭帧中断 
    DCMI_Stop();		//关闭传输
		break;
	}
  
	}    
}

void jpeg_picture(void)
{
	u8 *p;
//	size=5;		//默认是QVGA 320*240尺寸 
  jpeg_data_ok=0;   //初始化，再一次进行拍照
	
 	OV2640_JPEG_Mode();		//JPEG模式
	My_DCMI_Init();			//DCMI配置
	DCMI_DMA_Init((u32)&jpeg_buf,jpeg_buf_size,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA配置   
	OV2640_OutSize_Set(jpeg_img_size_tbl[pic_size][0],jpeg_img_size_tbl[pic_size][1]);//设置输出尺寸 
	DCMI_Start(); 		//启动传输
	p=(u8*)jpeg_buf;
	while(1)
	{
		delay_us(100);
		LED1=!LED1;	
	if(jpeg_data_ok==1)
	{		
		  Msp8266_start();  //开始校验 用于tcp校验
		
      M8266WIFI_SPI_Send_BlockData(p, (u32)jpeg_data_len*4, 500, 0, NULL, 0, &status);
		  delay_us(100);
			Msp8266_end();   //结束校验，用于tcp结束传输
		
		//	 Communication Test (Chinese: WIFI套接字的数据收发通信测试)
		jpeg_data_ok=3;    //设置为3，不再启用中断，并且不再对数据进行处理
//    DCMI_ITConfig(DCMI_IT_FRAME,DISABLE);//关闭帧中断 
//    DCMI_Stop();		//关闭传输
		break;
	}
	} 	
}
void jpeg_video(void)  //传输视频数据
{
	u8 *p;
//	size=5;		//默认是QVGA 320*240尺寸 
	jpeg_data_ok=0;   //初始化，再一次进行拍照
	
	
 	OV2640_JPEG_Mode();		//JPEG模式
	My_DCMI_Init();			//DCMI配置
	DCMI_DMA_Init((u32)&jpeg_buf,jpeg_buf_size,DMA_MemoryDataSize_Word,DMA_MemoryInc_Enable);//DCMI DMA配置   
	OV2640_OutSize_Set(jpeg_img_size_tbl[pic_size][0],jpeg_img_size_tbl[pic_size][1]);//设置输出尺寸 
	DCMI_Start(); 		//启动传输
	
	p=(u8*)jpeg_buf;

	
	while(1)
	{
		delay_us(100);
		LED1=!LED1;	
	if(jpeg_data_ok==1)
	{		
		
      M8266WIFI_SPI_Send_BlockData(p, (u32)jpeg_data_len*4, 5000, 0, NULL, 0, &status);
		  delay_us(100);

		
		jpeg_data_ok=2;	//标记jpeg数据处理完了,可以让DMA去采集下一帧了.
	if (M8266WIFI_SPI_Has_DataReceived()==1) //如果wifi收到了数据
	{
		jpeg_data_ok=3;    //设置为3，不再启用中断，并且不再对数据进行处理
//    DCMI_ITConfig(DCMI_IT_FRAME,DISABLE);//关闭帧中断 
//    DCMI_Stop();		//关闭传输
		break;
	}
	}
	} 	
}
//发送测试数据验证丢包率   测试过程中出现发送数据为0 ,现在考虑发送一个2048字节的数据，数据有特定规律，对比接收的字节数来确定丢包率
void Msp8266_packet_loss_detect(void)
{
	u8 *p;
	int i;
	u8 snd_data[256]; //设计一个256长度的数组，数据分为4次发送，发送都是0-255，之后校验每次接收程度,求丢包率平均值
	p=(u8*)snd_data;
	for(i=0;i<256;i++)
		snd_data[i]=i;

	M8266WIFI_SPI_Send_BlockData(p,sizeof(snd_data), 5000, 0, NULL, 0, &status);
	delay_us(100);
}
//回复命令，表示收到，并开始执行命令
void Msp8266_send_ok(void)
{
  u8 ok[]="ok!";
	u8 len=sizeof(ok);
//	u8 len1=sizeof(*Msp8266_oder[1]);  情况测试
	
	M8266WIFI_SPI_Send_BlockData(ok,len, 5000, 0, NULL, 0, &status);
}


void Msp8266_Initial_verification(void)  //数据初始AT+校验
{
	
}

u8 string_check(u8 order[],u8 Initial[],u8 len) //order为输入命令 Initial为校验命令 len为字符串长度
{
	u8 i;
	for(i=0;i<len;i++)
	{
	if(order[i]!=Initial[i])	
		return 0;
	}

	return 1;
}
//指令查询和执行
void Msp8266_oder_inquire_do(u16 rec,u8* rec_data)  //rec为数据长度 rec_data为实际数据
{
	u8 i=0,status_check=0;
	u8 Initial_data[3]="AT+";   //这个问题好像解决了
	u8 buf[15];  //用来存储去掉at+的其余指令

	for(i=0;i<=2;i++)  //校验前三位数
	{
		if ((Initial_data[i]!=rec_data[i])||(rec-3<=0))
		{
			status_check=1; //表示系统校验不通过
			break;
		}
	}
	if(status_check==1)  //校验不通过时发送error oder！
	{
  delay_us(80);
		
  M8266WIFI_SPI_Send_BlockData(Error_ODER,sizeof(Error_ODER), 5000, 0, NULL, 0, &status); //数据返回，反馈收到的数据	
//3.8 存在一个诡异的问题，同样的数据需要多发一遍才会有效。。。 感觉是中断里别存太多东西

		//考虑到如果无法多个字符串存在一个变量里，那么就考虑指令校验多个命令
		//问题已解决，是debug软件没读出数据
	//3.8修改意见 由于无法采用指令集的方式发送数据，所以考虑只写单条指令			
	}
	else
	{
	for(i=3;i<rec;i++)  //校验通过以后存储其余命令数据
		buf[i-3]=rec_data[i];
		if (string_check(buf,Picture,7))  //比较是否为拍照命令 picture字符串长度为7
		{
			 delay_us(20);
//		Msp8266_start_send();  //数据传输测试
	   delay_ms(50);
		jpeg_picture();  //拍一次照			
		}	
		else if(string_check(buf,Reset,5))     //字符串长度为5
		{
		__set_FAULTMASK(1); // 复位函数
    NVIC_SystemReset(); // 
		}
		else if(string_check(buf,Video,5))   //字符串长度为5
		{
			jpeg_video();
		}
		else if(string_check(buf,stop,4))
		{
		 delay_ms(10);  //什么都不做，回到等候指令的状态	
		}
		else if(string_check(buf,device_status_check,12)) //系统状态监测，展示系统外设状态和系统运行时间或者系统丢包率测试
		{
//			u8 tem[]="Device id is  ";
			u8 tem1[]="ov2640 is ready\n";		
			u8 len;
//			u8 len=sizeof(tem);
//			u8 len1;
//			M8266WIFI_SPI_Send_BlockData(tem, len, 5000, 0, NULL, 0, &status); //发送前言
//			delay_us(20);
//			len1=sizeof(device_id);
//			M8266WIFI_SPI_Send_BlockData(device_id, len1, 5000, 0, NULL, 0, &status); //发送系统id
//			delay_us(20);
      while(OV2640_Init())
			{
		  LED_set(1, 0); 
		  LED_set(1, 1); 	
		  delay_ms(200);
			}		
      len=sizeof(tem1);
      M8266WIFI_SPI_Send_BlockData(tem1, len, 5000, 0, NULL, 0, &status); //发送ov2640准备就绪		
      delay_us(20);
//      Msp8266_packet_loss_detect();			//系统丢包率监测  //考虑到丢包率监测较为复杂。。暂时考虑一下再去写4.29		
		}
		else if(string_check(buf,packet_loss_check,17)) //系统丢包率监测，详细说明见上面
		{
			Msp8266_packet_loss_detect();
		}
		else if(string_check(buf,size_small,5))   //设计图片尺寸大小
		{
		  pic_size=4;	
		}
		else if(string_check(buf,size_medium,5))//中
		{
			pic_size=6;
		}
		else if(string_check(buf,size_big,5))//大
		{
			pic_size=8;
		}
		else if(string_check(buf,shake_hands,5)) //握手信号判断
		{
		  Msp8266_send_ok();
		}
		else if(string_check(buf,Device,6))
		{
			u8 len2=sizeof(device_id);
			M8266WIFI_SPI_Send_BlockData(device_id, len2, 5000, 0, NULL, 0, &status); //发送系统id
		}	
	}
}


// 设置MSP8266为客户端TCP 
u8 Msp8266_Init(void)
{
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	 Setup Connection and Config connection upon neccessary (Chinese: 创建套接字，以及必要时对套接字的一些配置)
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	 
	 u16 status = 0;
	 u8  link_no=0;
//step 0: config tcp windows number (Chinese: 步骤0：如果是TCP类型的套接字，可以配置调整窗口参数）

// u8 M8266WIFI_SPI_Config_Tcp_Window_num(u8 link_no, u8 tcp_wnd_num, u16* status)

 if(M8266WIFI_SPI_Config_Tcp_Window_num(link_no, 4, &status)==0)
  {
     LED0=1;
		 delay_ms(100);
		 return 0;
	}
	LED0=0;

//step 1: setup connection (Chinese: 步骤1：创建套接字连接）
  //  u8 M8266WIFI_SPI_Setup_Connection(u8 tcp_udp, u16 local_port, char remote_addr, u16 remote_port, u8 link_no, u8 timeout_in_s, u16* status);
	if(M8266WIFI_SPI_Setup_Connection(TEST_CONNECTION_TYPE, TEST_LOCAL_PORT, TEST_REMOTE_ADDR, TEST_REMOTE_PORT, link_no, 20, &status)==0)
	{		
     LED1=1;
		 delay_ms(100);
		 return 0;
	}
	LED1=0;
	      // setup connection successfully, we could config it (Chinese: 创建套接字成功，就可以配置套接字）
 return 1;  //表示成功
}
//接收中断初始化
void Msp8266_Receive_exit_Init()
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;
	EXTI_InitTypeDef   EXTI_InitStructure;	

  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
 
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; //wifi的外部中断
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//普通输入模式
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100M
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;//下拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化GPIOA3   GPIO初始化
	
		 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//使能SYSCFG时钟
	
	SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOA, EXTI_PinSource3);//PA2 连接到中断线3
	
	/* 配置EXTI_Line2 */
	EXTI_InitStructure.EXTI_Line = EXTI_Line3 ;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;//中断事件
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //上升沿触发
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;//中断线使能
  EXTI_Init(&EXTI_InitStructure);//配置
 
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;//外部中断3
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x02;//抢占优先级1
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x02;//子优先级2
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
  NVIC_Init(&NVIC_InitStructure);//配置 
}

void EXTI3_IRQHandler(void)   //接收中断 系统按键接收，命令判断和执行  
{

	u8  link_no=0;	
   LED0=!LED0;  
	if (M8266WIFI_SPI_Has_DataReceived()==1) //如果wifi收到了数据
	{
	 received=M8266WIFI_SPI_RecvData(RecvData, RECV_DATA_MAX_SIZE,2000,0,&status); 

//		Msp8266_send_ok(); //发送ok的指令，表示数据收到
		
		Search_flag=1;
		
		
		
	Msp8266_oder_inquire_do(received,RecvData);
		
		
//	 M8266WIFI_SPI_Send_BlockData(RecvData,received, 5000, 0, NULL, 0, &status); //数据返回，反馈收到的数据
		
		//3.8 存在的问题，同样的数据需要再发一遍才会有效。。。 
		
		//问题已解决，是因为debug软件没转换
	 EXTI_ClearITPendingBit(EXTI_Line2);//清除LINE2上的中断标志位
	}		
}	


