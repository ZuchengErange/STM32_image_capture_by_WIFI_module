#ifndef __MSP8266_H
#define __MSP8266_H
#include "sys.h"

void jpeg_data_process(void);  //ͼ�δ�����
void jpeg_test(void);   //jpeg���Ժ���(�ƻ�ʵ������һ��)

void jpeg_picture(void); //jpeg���պ���������һ����Ƭ��
void jpeg_video(void);  //������Ƶ����



void Msp8266_send_ok(void);//�ظ������ʾ�յ�������ʼִ������

void Msp8266_oder_inquire_do(u16 rec,u8* rec_data); //����У��

u8 Msp8266_Init(void); // ����MSP8266Ϊ�ͻ���TCP 
	

void Msp8266_packet_loss_detect(void);//���Ͳ���������֤������
void Msp8266_Receive_exit_Init(void);//�����жϳ�ʼ��


#define ov2640_mode 1 //����ģʽ:0,RGB565ģʽ;1,JPEGģʽ
#endif
