#ifndef __MSP8266_H
#define __MSP8266_H
#include "sys.h"

void jpeg_data_process(void);  //图形处理函数
void jpeg_test(void);   //jpeg测试函数(计划实现拍照一次)

void jpeg_picture(void); //jpeg拍照函数（传输一次照片）
void jpeg_video(void);  //传输视频数据



void Msp8266_send_ok(void);//回复命令，表示收到，并开始执行命令

void Msp8266_oder_inquire_do(u16 rec,u8* rec_data); //命令校验

u8 Msp8266_Init(void); // 设置MSP8266为客户端TCP 
	

void Msp8266_packet_loss_detect(void);//发送测试数据验证丢包率
void Msp8266_Receive_exit_Init(void);//接收中断初始化


#define ov2640_mode 1 //工作模式:0,RGB565模式;1,JPEG模式
#endif
