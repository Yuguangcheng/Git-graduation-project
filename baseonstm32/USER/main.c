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
 ALIENTEKս��STM32������ʵ��40
 ������ʾ ʵ�� 
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/
	/* �û�����ǰ�豸״̬�ṹ��*/
dataPoint_t currentDataPoint;

void Gizwits_Init(void)
{	
	TIM3_Int_Init(9,7199);//1MSϵͳ��ʱ
  usart3_init(9600);//WIFI��ʼ��
	memset((uint8_t*)&currentDataPoint, 0, sizeof(dataPoint_t));//�豸״̬�ṹ���ʼ��
	gizwitsInit();//��������ʼ��
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
	uint8_t data[3]={0,0,0};//������ʷֵ�˲�
//��֪��Ϊʲô��main������������bugʹ�ó������㷨�������ƽ��ֵʱ�ܷɣ�������c8t6��Сϵͳ��ȴ��������������main��������
	uint32_t aun_ir_buffer[BUFFER_SIZE]; //infrared LED sensor data
	uint32_t aun_red_buffer[BUFFER_SIZE];  //red LED sensor data
	


 int main(void)
 {
  int buffer_heart[10]={0},buffer_spo2[10]={0},d=0,s=0,buffer_setheartH[10],buffer_setheartL[10],buffer_setspo2L[10],buffer_setspo2H[10]; 
	u32 fontcnt;		  
	u8 i,h=0,z=0;
	u8 fontx[2];//gbk��
	u8 key,key_flag;   	    
  uint8_t uch_dummy;
  u8 lcd_id[12];			//���LCD ID�ַ���
	 
	delay_init();	    	 //��ʱ������ʼ��	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
 	usmart_dev.init(72);		//��ʼ��USMART		
 	LED_Init();		  			//��ʼ����LED���ӵ�Ӳ���ӿ�
	KEY_Init();					//��ʼ������
	LCD_Init();			   		//��ʼ��LCD   
	W25QXX_Init();				//��ʼ��W25Q128
 	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
	exfuns_init();				//Ϊfatfs��ر��������ڴ�
  Gizwits_Init();
	 //max30102��ʼ������ 
	max30102_reset(); //resets the MAX30102
  maxim_max30102_read_reg(REG_INTR_STATUS_1,&uch_dummy);  //Reads/clears the interrupt status register
  max30102_init();  //initialize the MAX30102
	//��ID
	max_id= max30102_Bus_Read(REG_PART_ID);
	
 	f_mount(fs[0],"0:",1); 		//����SD�� 
 	f_mount(fs[1],"1:",1); 		//����FLASH.
	
	while(font_init()) 			//����ֿ�
	{
UPD:    
		LCD_Clear(WHITE);		   	//����
 		POINT_COLOR=RED;			//��������Ϊ��ɫ	   	   	  
		LCD_ShowString(30,50,200,16,16,"WarShip STM32");
		while(SD_Init())			//���SD��
		{
			LCD_ShowString(30,70,200,16,16,"SD Card Failed!");
			delay_ms(200);
			LCD_Fill(30,70,200+30,70+16,WHITE);
			delay_ms(200);		    
		}								 						    
		LCD_ShowString(30,70,200,16,16,"SD Card OK");
		LCD_ShowString(30,90,200,16,16,"Font Updating...");
		key=update_font(20,110,16,"0:");//�����ֿ�
		while(key)//����ʧ��		
		{			 		  
			LCD_ShowString(30,110,200,16,16,"Font Update Failed!");
			delay_ms(200);
			LCD_Fill(20,110,200+20,110+16,WHITE);
			delay_ms(200);		       
		} 		  
		LCD_ShowString(30,110,200,16,16,"Font Update Success!   ");
		delay_ms(1500);	
		LCD_Clear(WHITE);//����	       
	}
	
	POINT_COLOR=RED;       
	//background
  LCD_BackGround();
//	Show_Str(10,250,200,16,"�밴ѹ������...",16,0);
   currentDataPoint.valueset_heartH=100;//��ʼ���趨��ֵ
	 currentDataPoint.valueset_heartL=60;
	 currentDataPoint.valueset_spo2H=100;
	 currentDataPoint.valueset_spo2L=64; 
	while(1)
	{
    key = KEY_Scan(1);
		if(key==KEY2_PRES)//KEY2����
		{
			printf("WIFI����AirLink����ģʽ\r\n");
			gizwitsSetMode(WIFI_AIRLINK_MODE);//Air-linkģʽ����
		}			
		if(key==KEY1_PRES)//KEY_UP����
		{ 
			if(key_flag==1)
      sub(h-1,&currentDataPoint.valueset_heartH,&currentDataPoint.valueset_heartL,&currentDataPoint.valueset_spo2H,&currentDataPoint.valueset_spo2L);			
			LCD_Fill(550,300,710,390,WHITE);
		//LCD_Clear(WHITE); 
		threshold(currentDataPoint.valueset_heartH,currentDataPoint.valueset_heartL,currentDataPoint.valueset_spo2H,currentDataPoint.valueset_spo2L);
			//			printf("WIFI��λ����������������\r\n");
//			gizwitsSetMode(WIFI_RESET_MODE);//WIFI��λ
		}
//		delay_ms(200);
//		LED0=!LED0; 
		threshold(currentDataPoint.valueset_heartH,currentDataPoint.valueset_heartL,currentDataPoint.valueset_spo2H,currentDataPoint.valueset_spo2L);
	if(key==KEY0_PRES)//ѡ���趨����ֵ
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
		max30102_FIFO_ReadBytes(REG_FIFO_DATA,temp);//��MAX30102������
		//��ɫ���ֽ����֣��������ֽ�����
	  
		aun_ir_buffer[i] =  (long)((long)((long)temp[0]&0x03)<<16) | (long)temp[1]<<8 | (long)temp[2];    // Combine values to get the actual number
		aun_red_buffer[i] = (long)((long)((long)temp[3] & 0x03)<<16) |(long)temp[4]<<8 | (long)temp[5];  
			//if(b>50)
				//printf(" %d",aun_ir_buffer[i]);
		if(aun_ir_buffer[i]>20000)//һ�㰴��ʱ���ݶ���10w���ϣ�����С��2w���趨Ϊ��Ч���ݣ���⵽����b++�������ӵ�ǰ�治�ȶ���1s��50������
		{b++;
			if(b>80)//��ֹb���
				b=51;//����51����
		}
		else//����ʱb���0��Ϊ��һ�ΰ�����ǰ��������׼��
		{
			b=0;
			i=BUFFER_SIZE;//����ָ������ִ�и�ѭ��
			aun_ir_buffer[0]=0;//��ֹ������㷨ͨ��
			aun_ir_buffer[80]=0;//��for�˷ѣ���ֹ�����㷨ͨ��
			aun_ir_buffer[35]=0;
			aun_ir_buffer[20]=0;
			aun_ir_buffer[50]=0;
			aun_ir_buffer[149]=0;
			aun_ir_buffer[100]=0;
		}
		
		//�������ӵ���һ��ʼ����ʱ���䲻�ȶ���ǰ50�����ݣ�Ҳ����˵��һ�μ��ʱ��4s�����涼��3s����Ȼ�����Ӵ�������ʱû��ѹ��������Ʈ���㷨��ͨ��������ʾ
		if(b==50)//ÿ�ΰ���ʱ�ӵ�ǰ���50������������Ͳ�����
		{i=-1;
		 b=51;
		}
		
	}	
  
	  //calculate heart rate and SpO2 after BUFFER_SIZE samples (ST seconds of samples) using Robert's method
  
	//�����Ч��ʼ��������Ѫ��
	  //calculate heart rate and SpO2 after BUFFER_SIZE samples (ST seconds of samples) using Robert's method
rf_heart_rate_and_oxygen_saturation(aun_ir_buffer, BUFFER_SIZE, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid, &ratio, &correl); 
  //��ʱ�򣨼��͸��ʣ���֪��Ϊɶ�ᵯ��һ�����ֺ�һ˲��hr=40������ݣ����Լ�������ж�
	//����Ϊfor�������ֲ�ִ���ˣ�����ԭ����ֵ������Ȼ���㷨ִ��ͨ���ˣ�������ü������������ำֵ����0
	if(aun_ir_buffer[0]<70000||aun_ir_buffer[80]<70000||aun_ir_buffer[110]<70000||aun_ir_buffer[149]<70000)
		ch_hr_valid=0;
				
	 			if((ch_hr_valid == 1)&&(ch_spo2_valid == 1))
				{
					//��ʷֵ�˲�
					data[0]=data[1];
					data[1]=data[2];
					data[2]=n_heart_rate;
					if(data[1]>20&&data[0]>20)//����0����
					 n_heart_rate=(data[0]+data[1]+data[2])/3;
					 currentDataPoint.valueheart=n_heart_rate;
		       currentDataPoint.valuespo2=n_spo2;
					 buffer_heart[s]=n_heart_rate;
					 buffer_spo2[s]=n_spo2;
					
					 if(s>=1)//��ʼ��ͼ
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
					//�����Ʊ���
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
				//�������ϱ�����Ѫ���ͱ���
				//������ʱ�䵽����ִ���������������Ż��ϱ���ֻ��ʱ�䵽����û��ִ�е���������Ҳ�����ϱ�
			//	userHandle();
				gizwitsHandle((dataPoint_t*)&currentDataPoint);
				if(d==10)  
					d=0;
			  if(s==10)
				{
					s=0;
					for(i=0;i<9;i++) //������Ϊ��
					buffer_heart[i]=0;
						LCD_Clear(WHITE);   //���۱���ɫ
					 LCD_BackGround();
					
				}
		
}
	
	}



















