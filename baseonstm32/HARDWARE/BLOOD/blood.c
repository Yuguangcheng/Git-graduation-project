#include "sys.h"
#include "blood.h"
#include "max30102.h"
#include "math.h"
#include "lcd.h"
#include "usart.h"
#include "text.h"
#include "algorithm.h"
#include "led.h"
#include "usmart.h" 
#include "malloc.h"
#include "sdio_sdcard.h"  
#include "w25qxx.h"    
#include "ff.h"  
/*u8 temp_Hp[20];
u8 temp_HpO2[20];

uint16_t g_fft_index = 0;         	 	//fft输入输出下标

struct compx s1[FFT_N+16];           	//FFT输入和输出：从S[1]开始存放，根据大小自己定义
struct compx s2[FFT_N+16];           	//FFT输入和输出：从S[1]开始存放，根据大小自己定义

struct
{
	float 	Hp	;			//血红蛋白	
	float 	HpO2;			//氧合血红蛋白
	
}g_BloodWave;//血液波形数据

BloodData g_blooddata = {0};					//血液数据存储

#define CORRECTED_VALUE		47   			//标定血液氧气含量


//血液检测信息更新
void blood_data_update(void)
{
	u8 i;
	uint16_t temp_num=0;
	uint16_t fifo_word_buff[1][2];
	
	temp_num = max30100_Bus_Read(INTERRUPT_REG);
	
	//标志位被使能时 读取FIFO
	if (INTERRUPT_REG_A_FULL&temp_num)
	{
		GPIO_SetBits(GPIOB,GPIO_Pin_5);//HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,1);
		
		//读取FIFO
		max30100_FIFO_Read(0x05,fifo_word_buff,1);//read the hr and spo2 data form fifo in reg=0x05
		
		//将数据写入fft输入并清除输出
		for(int i = 0;i < 1;i++)
		{
			if(g_fft_index < FFT_N)
			{
				s1[g_fft_index].real = fifo_word_buff[i][0];
				s1[g_fft_index].imag= 0;
				s2[g_fft_index].real = fifo_word_buff[i][1];
				s2[g_fft_index].imag= 0;
				g_fft_index++;
			}
		}
		//信息更新标志位
		g_blooddata.update++;
	}
	else
	{
		GPIO_ResetBits(GPIOB,GPIO_Pin_5);//HAL_GPIO_WritePin(GPIOC,GPIO_PIN_13,0);
	}
}

//血液信息转换
void blood_data_translate(void)
{	
	//缓冲区写入结束
	if(g_fft_index>=FFT_N)
	{
//		//开始变换显示
//		Gui_DrawFont_GBK16(102,2,BLACK,GREEN,"FFT");
		LED0=0;
		
		g_fft_index = 0;
		
		//数据更新标志位
		g_blooddata.display = 1;
		
		//快速傅里叶变换
		FFT(s1);
		FFT(s2);
		
		//解平方
		for(int i = 0;i < FFT_N;i++) 
		{
			s1[i].real=sqrtf(s1[i].real*s1[i].real+s1[i].imag*s1[i].imag);
			s2[i].real=sqrtf(s2[i].real*s2[i].real+s2[i].imag*s2[i].imag);
		}
		
		//读取峰值点 结果的物理意义为 
		uint16_t s1_max_index = find_max_num_index(s1, 60);
		uint16_t s2_max_index = find_max_num_index(s2, 60);
		
		//检查HbO2和Hb的变化频率是否一致
		if(s1_max_index == s2_max_index)
		{
			//心率计算
			uint16_t Heart_Rate = 60 * SAMPLES_PER_SECOND * 
														s2_max_index / FFT_N;
			
			g_blooddata.heart = Heart_Rate - 10;
			
			//血氧含量计算
			float sp02_num = (s2[s1_max_index].real * s1[0].real)
											/(s1[s1_max_index].real * s2[0].real);
			
			sp02_num = (1 - sp02_num) * SAMPLES_PER_SECOND + CORRECTED_VALUE;
			
			g_blooddata.SpO2 = sp02_num;
			
			//状态正常
			g_blooddata.state = BLD_NORMAL;
		}
		else //数据发生异常
		{
			g_blooddata.heart = 0;
			g_blooddata.SpO2 	= 0;
			g_blooddata.state = BLD_ERROR;
		}
		//结束变换显示
//		Gui_DrawFont_GBK16(102,2,GREEN,BLACK,"FFT");
		LED0=1;
	}
}

//血液数据更新
void blood_wave_update(void)
{
	static DC_FilterData hbdc = {.w = 0,.init = 0,.a = 0};
	static DC_FilterData hbodc = {.w = 0,.init = 0,.a = 0};
	
	float hbag = 0;
	float hboag = 0;
	
	float hbag_d = 0;
	float hboag_d = 0;
	
	//前8次求平均值
	for(int i = 0;i < 8;i++)
	{
		hbag  += s1[g_fft_index - 8 + i].real;
		hboag += s2[g_fft_index - 8 + i].real;
	}
		
	//直流滤波
	hbag_d =  dc_filter(hbag,&hbdc) / 8;
	hboag_d = dc_filter(hboag,&hbodc) /8;
	
	//高度数据
	float hbhight  = 0;
	float hbohight = 0;
	
	//比例与偏置
	hbhight   = (-hbag_d / 8.0) + 10;
	hbohight  = (-hboag_d / 8.0)+10;
	
	//高度数据幅度限制
	hbhight = (hbhight > 103) ? 103 : hbhight;
	hbhight = (hbhight < 0) ?  0 : hbhight;
	
	hbohight = (hbohight > 103) ? 103 : hbohight;
	hbohight = (hbohight < 0) ?  0 : hbohight;
		
	//将数据发布到全局
	g_BloodWave.Hp = hbhight;
	g_BloodWave.HpO2 = hbohight;
	
	//血液状态检查
	if(hbag < 5000 ||hbag < 5000)
	{
		//清空数据
		g_blooddata.heart = 0;
		g_blooddata.SpO2 	= 0;
		
		//删除数据
		g_fft_index -= 7;
		
		if(g_blooddata.state != BLD_ERROR)
		{
			g_blooddata.state = BLD_ERROR;
			g_blooddata.display = 1;
		}
	}
	else
	{
		if(g_blooddata.state == BLD_ERROR)
		{
			g_blooddata.state = BLD_NORMAL;
			g_blooddata.display = 1;
		}
	}	
}
*/


void LCD_BackGround(void)
{
	LCD_Clear(WHITE);   //护眼背景色
	POINT_COLOR=RED;	  //
	Show_Str(400,10,200,16,"正常心率值为60-----100",16,0);	//Show_Str(x,y,width,eight,u8*str,u8 size,u8 mode)
	Show_Str(400,30,250,16,"正常血氧饱和度为94-----100",16,0);
	Show_Str(0,0,200,16,"心率血氧检测...",16,0);//LCD_ShowString( x, y,width,heightsize, *p)
	LCD_ShowString(270,75,200,15,16,"heart:");
	LCD_ShowString(270,180,200,16,16,"SpO2:");
	Show_Str(10,250,200,16,"请按压传感器...",16,0);
	LCD_DrawRectangle(4,20,318,238);//画框图
	LCD_DrawRectangle(6,22,260,125);
	LCD_DrawRectangle(6,126,260,236);
	LCD_DrawRectangle(262,22,316,125);
	LCD_DrawRectangle(262,126,316,236);
	//void LCD_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);							//画线
	LCD_DrawLine(20, 100, 240, 100);  //x轴 心率
	LCD_DrawLine(20, 40, 25, 45);
  LCD_DrawLine(20, 40, 15, 45);	
	LCD_DrawLine(20, 100, 20, 40);   //y轴   心率
	LCD_DrawLine(240, 100, 235, 105);
  LCD_DrawLine(240, 100, 235, 95);
  //
	LCD_DrawLine(20, 220, 240, 220);  //x轴 心率
	LCD_DrawLine(20, 160, 25, 165);
 LCD_DrawLine(20, 160, 15, 165);	
	LCD_DrawLine(20, 220, 20, 160);   //y轴   心率
  LCD_DrawLine(240, 220, 235, 215);
  LCD_DrawLine(240, 220, 235, 225);
}
void LCD_Judgeheart(uint8_t n)
{
	if(n>60&&n<=100)
	{
		Show_Str(400,80,200,16,"您的心率属于正常范围",16,0);
    Show_Str(400,100,600,16,"常参加各种强度的适宜的运动,忌熬夜、酗酒、吸烟",16,0);
    Show_Str(400,120,600,16,"合理调节膳食，保持适当体重",16,0);		
	}
	else if(n<60)
	{
		Show_Str(400,80,200,16,"您的心率偏低",16,0);
    Show_Str(400,100,600,16,"常参加各种强度的适宜的运动,忌熬夜、酗酒、吸烟",16,0);
    Show_Str(400,120,600,16,"合理调节膳食，保持适当体重",16,0);		
	}
	else if(n>100)
	{
		Show_Str(400,80,200,16,"您的心率偏高",16,0);
    Show_Str(400,100,600,16,"常参加各种强度的适宜的运动,忌熬夜、酗酒、吸烟",16,0);
    Show_Str(400,120,600,16,"合理调节膳食，保持适当体重",16,0);		
	}
}
void LCD_Judgespo2(uint8_t n)
{
	if(n<=100&&n>94)
	{
		Show_Str(400,200,200,16,"您的血氧属于正常范围",16,0);
    
	}
	if(n<94)
	{
		Show_Str(400,200,200,16,"您的血氧偏低",16,0);
    Show_Str(400,220,600,16,"注意保养，否则可能导致机体供氧不足",16,0);
	}
	if(n>100)
	{
		Show_Str(400,200,200,16,"您的血氧偏低",16,0);
    Show_Str(400,220,600,16,"注意保养，否则可能导致体内细胞老化",16,0);
	}
}
void threshold(int n,int p,int z,int y)
{
  Show_Str(400,300,150,16,"当前心率最高阈值：",16,0);LCD_DrawLine(550, 310,556, 310);LCD_DrawLine(710, 310,716, 310);LCD_DrawLine(713, 313,713, 307);LCD_DrawLine(560, 310,700, 310);
  LCD_Draw_Circle(565+n,310,5);	
	if(n>120) n=120;
	LCD_ShowNum(750,300,n,3,16); 
	
  Show_Str(400,320,150,16,"当前心率最低阈值：",16,0);LCD_DrawLine(550, 330,556, 330);LCD_DrawLine(710, 330,716, 330);LCD_DrawLine(713, 333,713, 327);LCD_DrawLine(560, 330,700, 330);
	LCD_Draw_Circle(565+p,330,5);	
	if(p<0) p=0;
	LCD_ShowNum(750,320,p,3,16);
	
  Show_Str(400,340,150,16,"当前血氧最高阈值:",16,0);LCD_DrawLine(550, 350,556, 350);LCD_DrawLine(710, 350,716, 350);LCD_DrawLine(713, 353,713, 347);LCD_DrawLine(560, 350,700, 350);
	LCD_Draw_Circle(565+z,350,5);	if(z>120) z=120;
	LCD_ShowNum(750,340,z,3,16);
	
  Show_Str(400,360,150,16,"当前血氧最低阈值:",16,0);;LCD_DrawLine(550, 370,556, 370);LCD_DrawLine(710, 370,716, 370);LCD_DrawLine(713, 373,713, 367);LCD_DrawLine(560, 370,700, 370);
	LCD_Draw_Circle(565+y,370,5);	if(y<0) y=0;
	LCD_ShowNum(750,360,y,3,16); 	
}
void draw_arrow(int n)
{
	switch(n)
	{
		case 0: LCD_DrawLine(340, 310, 390, 310);LCD_DrawLine(390, 310, 385, 315);LCD_DrawLine(390, 310, 385, 305);break;
		case 1: LCD_DrawLine(340, 330, 390, 330);LCD_DrawLine(390, 330, 385, 335);LCD_DrawLine(390, 330, 385, 325);break;
		case 2: LCD_DrawLine(340, 350, 390, 350);LCD_DrawLine(390, 350, 385, 355);LCD_DrawLine(390, 350, 385, 345);break;
		case 3: LCD_DrawLine(340, 370, 390, 370);LCD_DrawLine(390, 370, 385, 375);LCD_DrawLine(390, 370, 385, 365);break;
}
			
	}
void add(int n,int *x,int *y,int *z,int*w)
{
	switch(n)
	{
		case 0: (*x)++;break;
		case 1: (*y)++;break;
		case 2: (*z)++;break;
		case -1: (*w)++;break;
		
	}
}
void sub(int n,int *x,int *y,int *z,int*w)
{
	switch(n)
	{
		case 0: (*x)--;break;
		case 1: (*y)--;break;
		case 2: (*z)--;break;
		case -1: (*w)--;break;
		
	}
}






















