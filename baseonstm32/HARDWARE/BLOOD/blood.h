#ifndef __BLOOD_H
#define __BLOOD_H

#include "sys.h"

////////////////////////////////////
typedef enum
{
	BLD_NORMAL,		//����
	BLD_ERROR,		//������
	
}BloodState;//ѪҺ״̬


typedef struct
{
	uint16_t 		heart;		//��������
	float 			SpO2;			//Ѫ������
	BloodState	state;		//״̬
	uint8_t   	update;		//��Ϣ���±�־λ
	uint8_t   	display;	//���ݸ��±�־λ
}BloodData;


void blood_data_update(void);
void tft_display_update(void);
void blood_wave_update(void);
void blood_Setup(void);
void blood_Loop(void);
void LCD_BackGround(void);
void LCD_Judgeheart(uint8_t n);
void draw(int p[]);
void LCD_Judgespo2(uint8_t n);
void threshold(int n,int p,int z,int y);
void draw_arrow(int n);
void add(int n,int *x,int *y,int *z,int *w);
void sub(int n,int *x,int *y,int *z,int *w);
#endif



















