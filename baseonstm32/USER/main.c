#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h" 		 	 
#include "lcd.h"  
#include "key.h"     
#include "usmart.h" 
#include "malloc.h"
#include "sdio_sdcard.h"  
#include "w25qxx.h"    
#include "ff.h"  
#include "exfuns.h"   
#include "text.h"
#include "usart3.h"
#include "timer.h"
#include "gizwits_product.h"
#include "max30102.h" 
#include "myiic.h"
#include "algorithm_by_RF.h"
#include "blood.h"
 
 
/************************************************
 ALIENTEK战舰STM32开发板实验40
 汉字显示 实验 
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/
	/* 用户区当前设备状态结构体*/
dataPoint_t currentDataPoint;

void Gizwits_Init(void)
{	
	TIM3_Int_Init(9,7199);//1MS系统定时
  usart3_init(9600);//WIFI初始化
	memset((uint8_t*)&currentDataPoint, 0, sizeof(dataPoint_t));//设备状态结构体初始化
	gizwitsInit();//缓冲区初始化
}

int translate(int n)
{
	return (n/10)*16+n%10;
}

#define BUFFER_SIZE (FS*ST)
   uint8_t max_id;
	//Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every ST seconds
  float n_spo2,ratio,correl;  //SPO2 value
  int8_t ch_spo2_valid;  //indicator to show if the SPO2 calculation is valid
  int32_t n_heart_rate; //heart rate value
  int8_t  ch_hr_valid;  //indicator to show if the heart rate calculation is valid
  u8 h_hr;
  u8 l_hr;
  u8 l_spo2;
  int32_t i;
  char hr_str[10];
	uint8_t temp[6];
	int b=0;
	int a=0;
	u8 key;
	uint8_t data[3]={0,0,0};//用于历史值滤波
//不知道为什么在main里面声明会有bug使得程序在算法里面计算平均值时跑飞，但是在c8t6最小系统里却不会这样，能在main里面声明
	uint32_t aun_ir_buffer[BUFFER_SIZE]; //infrared LED sensor data
	uint32_t aun_red_buffer[BUFFER_SIZE];  //red LED sensor data
	


 int main(void)
 {
  int buffer_heart[10]={0},buffer_spo2[10]={0},d=0,s=0,buffer_setheartH[10],buffer_setheartL[10],buffer_setspo2L[10],buffer_setspo2H[10]; 
	u32 fontcnt;		  
	u8 i,h=0,z=0;
	u8 fontx[2];//gbk码
	u8 key,key_flag;   	    
  uint8_t uch_dummy;
  u8 lcd_id[12];			//存放LCD ID字符串
	 
	delay_init();	    	 //延时函数初始化	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200
 	usmart_dev.init(72);		//初始化USMART		
 	LED_Init();		  			//初始化与LED连接的硬件接口
	KEY_Init();					//初始化按键
	LCD_Init();			   		//初始化LCD   
	W25QXX_Init();				//初始化W25Q128
 	my_mem_init(SRAMIN);		//初始化内部内存池
	exfuns_init();				//为fatfs相关变量申请内存
  Gizwits_Init();
	 //max30102初始化操作 
	max30102_reset(); //resets the MAX30102
  maxim_max30102_read_reg(REG_INTR_STATUS_1,&uch_dummy);  //Reads/clears the interrupt status register
  max30102_init();  //initialize the MAX30102
	//读ID
	max_id= max30102_Bus_Read(REG_PART_ID);
	
 	f_mount(fs[0],"0:",1); 		//挂载SD卡 
 	f_mount(fs[1],"1:",1); 		//挂载FLASH.
	
	while(font_init()) 			//检查字库
	{
UPD:    
		LCD_Clear(WHITE);		   	//清屏
 		POINT_COLOR=RED;			//设置字体为红色	   	   	  
		LCD_ShowString(30,50,200,16,16,"WarShip STM32");
		while(SD_Init())			//检测SD卡
		{
			LCD_ShowString(30,70,200,16,16,"SD Card Failed!");
			delay_ms(200);
			LCD_Fill(30,70,200+30,70+16,WHITE);
			delay_ms(200);		    
		}								 						    
		LCD_ShowString(30,70,200,16,16,"SD Card OK");
		LCD_ShowString(30,90,200,16,16,"Font Updating...");
		key=update_font(20,110,16,"0:");//更新字库
		while(key)//更新失败		
		{			 		  
			LCD_ShowString(30,110,200,16,16,"Font Update Failed!");
			delay_ms(200);
			LCD_Fill(20,110,200+20,110+16,WHITE);
			delay_ms(200);		       
		} 		  
		LCD_ShowString(30,110,200,16,16,"Font Update Success!   ");
		delay_ms(1500);	
		LCD_Clear(WHITE);//清屏	       
	}
	
	POINT_COLOR=RED;       
	//background
  LCD_BackGround();
//	Show_Str(10,250,200,16,"请按压传感器...",16,0);
   currentDataPoint.valueset_heartH=100;//初始化设定阈值
	 currentDataPoint.valueset_heartL=60;
	 currentDataPoint.valueset_spo2H=100;
	 currentDataPoint.valueset_spo2L=64; 
	while(1)
	{
    key = KEY_Scan(1);
		if(key==KEY2_PRES)//KEY2按键
		{
			printf("WIFI进入AirLink连接模式\r\n");
			gizwitsSetMode(WIFI_AIRLINK_MODE);//Air-link模式接入
		}			
		if(key==KEY1_PRES)//KEY_UP按键
		{ 
			if(key_flag==1)
      sub(h-1,&currentDataPoint.valueset_heartH,&currentDataPoint.valueset_heartL,&currentDataPoint.valueset_spo2H,&currentDataPoint.valueset_spo2L);			
			LCD_Fill(550,300,710,390,WHITE);
		//LCD_Clear(WHITE); 
		threshold(currentDataPoint.valueset_heartH,currentDataPoint.valueset_heartL,currentDataPoint.valueset_spo2H,currentDataPoint.valueset_spo2L);
			//			printf("WIFI复位，请重新配置连接\r\n");
//			gizwitsSetMode(WIFI_RESET_MODE);//WIFI复位
		}
//		delay_ms(200);
//		LED0=!LED0; 
		threshold(currentDataPoint.valueset_heartH,currentDataPoint.valueset_heartL,currentDataPoint.valueset_spo2H,currentDataPoint.valueset_spo2L);
	if(key==KEY0_PRES)//选择设定的阈值
	{
		key_flag=1;
		LCD_Fill(300,300,395,375,WHITE);
		draw_arrow(h);
		h++;
		if(h==4)
		h=0;
	}
	if(key==WKUP_PRES)
	{
		if(key_flag==1)
		 add(h-1,&currentDataPoint.valueset_heartH,&currentDataPoint.valueset_heartL,&currentDataPoint.valueset_spo2H,&currentDataPoint.valueset_spo2L);
		LCD_Fill(550,290,710,390,WHITE);
		threshold(currentDataPoint.valueset_heartH,currentDataPoint.valueset_heartL,currentDataPoint.valueset_spo2H,currentDataPoint.valueset_spo2L);
	}
		//printf("***********");
  for(i=0;i<BUFFER_SIZE;i++)
  {
		while(MAX30102_INT==1);   //wait until the interrupt pin asserts;
		/*
		for(k=6;k>0;k--)
		{
			 LCD_ShowNum(10,270,k,20,16); 
			 delay_ms(1000);
		}
		*/
		max30102_FIFO_ReadBytes(REG_FIFO_DATA,temp);//从MAX30102读数据
		//红色三字节数字，红外三字节数据
	  
		aun_ir_buffer[i] =  (long)((long)((long)temp[0]&0x03)<<16) | (long)temp[1]<<8 | (long)temp[2];    // Combine values to get the actual number
		aun_red_buffer[i] = (long)((long)((long)temp[3] & 0x03)<<16) |(long)temp[4]<<8 | (long)temp[5];  
			//if(b>50)
				//printf(" %d",aun_ir_buffer[i]);
		if(aun_ir_buffer[i]>20000)//一般按下时数据都是10w以上，这里小于2w的设定为无效数据，检测到数据b++，方便扔掉前面不稳定的1s即50组数据
		{b++;
			if(b>80)//防止b溢出
				b=51;//大于51就行
		}
		else//松手时b变回0，为下一次按下扔前面样本做准备
		{
			b=0;
			i=BUFFER_SIZE;//松手指基本不执行该循环
			aun_ir_buffer[0]=0;//防止下面的算法通过
			aun_ir_buffer[80]=0;//用for浪费，防止下面算法通过
			aun_ir_buffer[35]=0;
			aun_ir_buffer[20]=0;
			aun_ir_buffer[50]=0;
			aun_ir_buffer[149]=0;
			aun_ir_buffer[100]=0;
		}
		
		//作用是扔掉你一开始按下时及其不稳定的前50组数据，也就是说第一次检测时间4s，后面都是3s，当然如果你接触传感器时没按压好数据乱飘则算法不通过，不显示
		if(b==50)//每次按下时扔掉前面的50组样本，后面就不扔了
		{i=-1;
		 b=51;
		}
		
	}	
  
	  //calculate heart rate and SpO2 after BUFFER_SIZE samples (ST seconds of samples) using Robert's method
  
	//如果有效开始计算心率血氧
	  //calculate heart rate and SpO2 after BUFFER_SIZE samples (ST seconds of samples) using Robert's method
rf_heart_rate_and_oxygen_saturation(aun_ir_buffer, BUFFER_SIZE, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid, &ratio, &correl); 
  //有时候（极低概率）不知道为啥会弹出一个松手后一瞬间hr=40多的数据，所以加下面的判断
	//是因为for那里松手不执行了，导致原来的值残留，然后算法执行通过了，所以最好加下面或者上面多赋值几个0
	if(aun_ir_buffer[0]<70000||aun_ir_buffer[80]<70000||aun_ir_buffer[110]<70000||aun_ir_buffer[149]<70000)
		ch_hr_valid=0;
				
	 			if((ch_hr_valid == 1)&&(ch_spo2_valid == 1))
				{
					//历史值滤波
					data[0]=data[1];
					data[1]=data[2];
					data[2]=n_heart_rate;
					if(data[1]>20&&data[0]>20)//大于0即可
					 n_heart_rate=(data[0]+data[1]+data[2])/3;
					 currentDataPoint.valueheart=n_heart_rate;
		       currentDataPoint.valuespo2=n_spo2;
					 buffer_heart[s]=n_heart_rate;
					 buffer_spo2[s]=n_spo2;
					
					 if(s>=1)//开始绘图
					 {
					 LCD_DrawLine(40+(s-1)*16,150-buffer_heart[s-1],40+s*16 ,150-buffer_heart[s]);
					 LCD_DrawLine(40+(s-1)*16,270-buffer_spo2[s-1],40+s*16 ,270-buffer_spo2[s]);
					 }
					 s+=1;
					 
           currentDataPoint.valueheart_hisdata[d]=translate(n_heart_rate);
		       currentDataPoint.valuespo2_hisdata[d]=translate(n_spo2);
					 d+=2;
					 LCD_Judgeheart(n_heart_rate);
					 LCD_Judgespo2(n_spo2);
		       LCD_ShowNum(270,95,n_heart_rate,3,16);	
		       LCD_ShowNum(270,200,(int)n_spo2,2,16);
					 //draw(buffer_heart);
					//LCD_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)
					//printf("hr=%d sp=%d",n_heart_rate,(int)n_spo2);
					//机智云报警
					if(n_heart_rate>currentDataPoint.valueset_heartH||n_heart_rate<currentDataPoint.valueset_heartL)
						currentDataPoint.valueheart_alarm=1;
					else
						currentDataPoint.valueheart_alarm=0;
					
					if(n_spo2< currentDataPoint.valueset_spo2L||n_spo2>currentDataPoint.valueset_spo2H)
					currentDataPoint.valuespo2_alarm=1;
					else
						currentDataPoint.valuespo2_alarm=0;
		
				}
				else
				{
					data[0]=0;
					data[1]=0;
					data[2]=0;
					n_heart_rate=0;
					n_spo2=0;
				//	printf("hr=%d sp=%d",n_heart_rate,(int)n_spo2);
	
			
				}
				//机智云上报心率血氧和报警
				//必须是时间到了且执行完这两个函数才会上报，只是时间到但是没有执行到两个函数也不会上报
			//	userHandle();
				gizwitsHandle((dataPoint_t*)&currentDataPoint);
				if(d==10)  
					d=0;
			  if(s==10)
				{
					s=0;
					for(i=0;i<9;i++) //重新置为零
					buffer_heart[i]=0;
						LCD_Clear(WHITE);   //护眼背景色
					 LCD_BackGround();
					
				}
		
}
	
	}



















