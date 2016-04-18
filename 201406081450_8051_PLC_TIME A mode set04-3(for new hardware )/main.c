/***************************************************************************/
/****************************Project:	8051_PLC_TIME **********************/
/****************************CopyLight: @ken *******************************/
/****************************Verson: V1.0 **********************************/
/****************************Date: 2014-06-28 ******************************/ 
/*************** 增加了功能：检测插卡和拔卡延时判断功能 *********************/
/***************************************************************************/

#include <STC11.H>
//#include "STC12C5052.h"


#define KEY_FRAME_01       			             150u            //毫秒

#define DELAY_TIME_MOTOR_L1                      12u			   //秒
#define DELAY_TIME_MOTOR_L2                      12u			   //秒

#define	Key_BIL1 P02
#define	Key_BIL2 P03
#define	Key_B01  0
#define Key_C01	 1
#define	Key_AIL1 P00
#define	Key_AIL2 P01
///////////////////////////////以下define 用于定义插卡方式，请选择一种，屏蔽其他的//////////////////////
#define Key_C02	 P14           // 只用  CARD_MODE B
//#define Key_C02	 P15           // 只用 CARD_MODE A
//#define Key_C02	 (P15||P14)           // 同时使用 CARD_MODE A 和 CARD_MODE B
//#define Key_C02    P25				//只使用SW1，即板上的26、27端口
//////////////////////////////////// end  /////////////////////////////////////////////////////////////////
#define LED5   P40					 
#define LED6   P41

#define Key_Frame_21              1u                   //检测确认插卡时间
#define Key_Frame_22              6u                   //检测确认拔卡时间
#define Key_Frame_23              20u                   //拔卡后留给客人操作的时间


#define 	AOL1					P20
#define 	AOL2					P21
#define     BOL1					P22
#define     BOL2					P23	  

#define 	NOBODY_L2_DOWN			1
#define     NOBODY_L1_DOWN          0

//int Key_Frame_21_time_flag=0;   //用于计算C02按键接触时间
int Key_Frame_23_time_flag=0;   //用于计算C01按键接触时间
//int Key_Frame_21_time=0;   //用于计算C02按键接触时间
int Key_Frame_23_time=0;   //用于计算C01按键接触时间

int Key_time_C02_On = 0;      //C02按制时间计数
int Key_time_C02_Off = 0;      //C02按制时间计数

int MOTOR_L1_Runtime = 0;         //AL1运行计时
int MOTOR_L1_Runtime_Offse = 0;   //用于记下从欢迎模式时拔卡后剩下AL1运行的时间

/////////////////////////////////////////////////

int cont = 0;                      //用于定时计数
int delay_1ms = 0;				   //用于计数延时1豪秒
int delay_ms = 0;
int delay_1s = 0;				   //用于计数延时1秒
int delay_1ms_for_100ms = 0; //用于计数延时100毫秒

unsigned int L1_runtime = 0;	   //运行计时
unsigned int L2_runtime = 0;

unsigned char key = 0;			   //用于返回按键值

unsigned char flag_AOL1 = 0;		   //用于标志按键之前是否已处于按下状态
unsigned char flag_AOL2 = 0; 
unsigned char flag_C01 = 0;
unsigned char flag_BOL1 = 0;
unsigned char flag_BOL2 = 0;
unsigned char flag_C02 = 0;
unsigned char welcome_flag = 0;

unsigned char L1_delay_flag = 0;  //用于标志C02按下首次L1运行进入延时（即进入欢迎模式）
/////////////////////////////////////延时函数/////////////////////////////////
void delay(int time)
{

	delay_1s = 0;
	do
	{
		;
	}
	while( delay_1s!=time );
}
void delayms(int time)
{
	delay_ms = 0;
	do
	{
		;
	}
	while( delay_ms !=time );
}
///////////////////////////////////马达停止///////////////////////////////////
void Motor_Stop(void)				          //停止马达运行
{
	AOL1 = 0;
	AOL2 = 0;
	BOL1 = 0;
	BOL2 = 0;
}
//////////////////////////////////获取按键函数///////////////////////////////
void GetKey(void)
{
			 //key用于标准按键状态，每位表示一个按键状态，0表示没按，1表示按下，
											 //0到6位分别表示A01到C02的状态
	if( Key_AIL1==0 )
	{
		delayms(KEY_FRAME_01);
		if( Key_AIL1==0 )
			key |= 0x01;
	}
	else
		key &= ~0x01;

	if( Key_AIL2==0 )
	{
		delayms(KEY_FRAME_01);
		if( Key_AIL2==0 )	
			key |= 0x02;
	}
	else
		key &= ~0x02;

	if( Key_C01==0 )
	{
		delayms(KEY_FRAME_01);
		if( Key_C01==0 )			
			key |= 0x04;
	}
	else
		key &= ~0x04;

	if( Key_BIL1==0 )
	{
		delayms(KEY_FRAME_01);
		if( Key_BIL1==0 )
			key |= 0x08;
	}
	else
		key &= ~0x08;

	if( Key_BIL2==0 )
   	{
		delayms(KEY_FRAME_01);
		if( Key_BIL2==0 )		
			key |= 0x10;
	}
	else
		key &= ~0x10;

// 	if( Key_C02==0 )
// 	{
// 		delayms(KEY_FRAME_01);
// 		if( Key_C02==0 )		
// 			flag_key |= 0x20;
// 	}
// 	else
// 		flag_key &= ~0x20;


// 	return flag_key;

}
//////////////////////////////////中断调用函数//////////////////////////////
/////////////////////////////////用于判断C02是否接上////////////////////////
////////////////////////有延时判断（拔插卡时间）////////////////////////////
void read_C02(void)
{
	if( Key_C02==0 )
	{
		Key_time_C02_On ++;
		Key_time_C02_Off = 0;
		if( Key_time_C02_On>=(int)(Key_Frame_21*10) )	  //判断按制时间是否足够	
		{	
			key |= 0x20;
			Key_time_C02_On =(int)(Key_Frame_21*10);
			MOTOR_L1_Runtime_Offse = 0;
			welcome_flag = 1;
		}
	}
	else
	{
		Key_time_C02_Off ++;
		Key_time_C02_On = 0;
		if( Key_time_C02_Off>=(int)(Key_Frame_22*10) )	  //判断不按制时间是否足够
		{
			key &= ~0x20;
			Key_time_C02_Off =(int)(Key_Frame_22*10);
			
			if((L1_delay_flag==1)&&(welcome_flag==1))
			{
				MOTOR_L1_Runtime_Offse =(int)( DELAY_TIME_MOTOR_L1 - MOTOR_L1_Runtime);
				welcome_flag = 0;
			}
		}
	}
}
//////////////////////////////////执行函数///////////////////////////////////
void run(void)
{
	if((flag_C02 ==0)&&( (key&0x20)==0x20 ) )				   //
	{		
		Key_Frame_23_time_flag = 0;
		Key_Frame_23_time = 0;
		flag_C02 = 1;				// C02已接上标志
		flag_AOL1 = 0;
		flag_AOL2 = 0;
		flag_BOL1 = 0;
		flag_BOL2 = 0;
		AOL2=0;
		AOL1=1;
		BOL2=0;
		BOL1=1;
		L1_delay_flag = 1;
		delay(DELAY_TIME_MOTOR_L1);                //延时180秒
		L1_delay_flag = 0;
		AOL1=0;
		BOL1=0;
		
	}
	else if((flag_C02 ==1)&&( (key&0x20)==0x20))
	{
		Key_Frame_23_time_flag = 0;
		Key_Frame_23_time = 0;
		flag_C02 = 1;				// C02已接上标志
		flag_AOL1 = 0;
		flag_AOL2 = 0;
		flag_BOL1 = 0;
		flag_BOL2 = 0;

		if( ((key&0x01)==0x01)&&((key&0x02)==0) )						 //A02接上
		{
			AOL2 = 0;
			AOL1 = 1;
		}
		else if ( (key&0x01)==0 )					 //A02没接上
			AOL1 = 0;
	  	else {	AOL1=0; AOL2=0;	}


	   	if ( ((key&0x02)==0x02)&&((key&0x01)==0) )					 //B02接上
		{
			AOL1 = 0;
			AOL2 = 1;
		}
		else if ( (key&0x02)==0 )					 //B02没接上
			AOL2 = 0;

		else {	AOL1=0; AOL2=0;	}


		if( ((key&0x08)==0x08)&&((key&0x10)==0) )						 //A02接上
		{
			BOL2 = 0;
			BOL1 = 1;
		}
		else if ( (key&0x08)==0 )					 //A02没接上
			BOL1 = 0;
	  	else {	BOL1=0; BOL2=0;	}


	   	if ( ((key&0x10)==0x10)&&((key&0x08)==0) )					 //B02接上
		{
			BOL1 = 0;
			BOL2 = 1;
		}
		else if ( (key&0x10)==0 )					 //B02没接上
			BOL2 = 0;

		else {	BOL1=0; BOL2=0;	}
	 }
	else if((flag_C02 ==1)&&(key&0x20)==0 &&(Key_Frame_23_time<=((Key_Frame_23 - MOTOR_L1_Runtime_Offse)>0?(Key_Frame_23 - MOTOR_L1_Runtime_Offse):0)))
	{
		Key_Frame_23_time_flag = 1;
		flag_C02 = 1;				// C02已接上标志
		flag_AOL1 = 0;
		flag_AOL2 = 0;
		flag_BOL1 = 0;
		flag_BOL2 = 0;

		if( ((key&0x01)==0x01)&&((key&0x02)==0) )						 //AIL1接上
		{
			AOL2 = 0;
			AOL1 = 1;
		}
		else if ( (key&0x01)==0 )					 //AIL1没接上
			AOL1 = 0;
	  	else {	AOL1=0; AOL2=0;	}


	   	if ( ((key&0x02)==0x02)&&((key&0x01)==0) )					 //AIL2接上
		{
			AOL1 = 0;
			AOL2 = 1;
		}
		else if ( (key&0x02)==0 )					 //AIL2没接上
			AOL2 = 0;

		else {	AOL1=0; AOL2=0;	}

		if( ((key&0x08)==0x08)&&((key&0x10)==0) )						 //BIL1接上
		{
			BOL2 = 0;
			BOL1 = 1;
		}
		else if ( (key&0x08)==0 )					 //BIL1没接上
			BOL1 = 0;
	  	else {	BOL1=0; BOL2=0;	}

	   	if ( ((key&0x10)==0x10)&&((key&0x08)==0) )					 //BIL2接上
		{
			BOL1 = 0;
			BOL2 = 1;
		}
		else if ( (key&0x10)==0 )					 //BIL2没接上
			BOL2 = 0;

		else {	BOL1=0; BOL2=0;	}

	}
	else if ( (key&0x20)==0 )
	{
		flag_C02 = 0;				// C02没接上标志
		Key_Frame_23_time_flag = 0;
		Key_Frame_23_time = 0;
//		Key_Frame_21_time = 0;
		if((flag_AOL1 == 0 )&&( flag_AOL2 == 0 )&&( flag_BOL1==0 )&&( flag_BOL2==0 )&&( Key_C02 )&&NOBODY_L1_DOWN )		 //A01接上
		{
			flag_AOL1 = 1;
			AOL2 = 0;
			AOL1 = 1;
			BOL2 = 0;
			BOL1 = 1;
			//delay(DELAY_TIME_MOTOR_L1 );
			L1_runtime = DELAY_TIME_MOTOR_L1;
	
		}
		else if ((key&0x01)==0)							 //A01没接上
		{
			flag_AOL1 = 0;
			flag_BOL1 = 0;

			if( L1_runtime==0 )
			{
				AOL1 = 0;
				BOL1 = 0;
			}			
		}

		else if( flag_AOL1==1 )  
		{
			if( L1_runtime==0 )
			{
				AOL1 = 0;
				BOL1 = 0;
			}			
		}
		else ;

		if((flag_BOL1 == 0 )&&(flag_BOL2 == 0 )&&( flag_AOL1==0 )&&( flag_AOL2==0 )&&( Key_C02 )&& NOBODY_L2_DOWN )	       //B01接上
		{
			flag_AOL2 = 1;
			AOL1 = 0;
			AOL2 = 1;
			BOL1 = 0;
			BOL2 = 1;
			//delay(DELAY_TIME_MOTOR_L2 );
			L2_runtime = DELAY_TIME_MOTOR_L2;
			
		}
		else if ((key&0x02)==0)							  //B01没接上
		{
			flag_BOL1 = 0;
			flag_AOL1 = 0;
			if( L2_runtime==0 )
			{
				AOL2 = 0;
				BOL2 = 0;
			}		
		}

		else if( flag_AOL2==1 ) 
		{
			if( L2_runtime==0 )
			{
				AOL2 = 0;
				BOL2 = 0;
			}	
						
		}
		else ;
	}
	else;	 
}
////////////////////////////////////////芯片初始化//////////////////////////////
void InitCPU(void)
{
	/*----- Initialize interrupt -------------*/
    TMOD = 0x22;		// 0010 0010 timer 0 - 8 bit auto reload		
						// timer 1 - baud rate generator

	TH1 =TL1 = 0xf3; 	//    SMOD = 0      SMOD =1						
						// 0ffh :57600 bps	0fdh :115200 bps			
						// 0fdh : 9600 bps	0fdh :19200 bps				
						// 0fah : 4800 bps	0fah :9600 bps							
						// 0f4h : 2400 bps								
						// 0e8h : 1200 bps	

				
    TCON = 0x55;		// 0101 0001 timer 0,1 run						
						// int 0,  edge triggered						
						// int 1,  edge triggered						
						// TF1 TR1 TF0 TR0	EI1 IT1 EI0 IT0	
	
	SCON = 0x50;		// 0100 0000 mode 1 - 8 bit UART				
						// Enable serial reception			

	TH0 = TL0 = 0x03;		// 250us

	PCON = 0x80;		// 0000 0000 SMOD(double baud rate bit) = 1		



	IP	 = 0x22;		// 0000 0000 interrupt priority					
						// -  - PT2 PS PT1 PX1 PT0 PX0	 

	IE   = 0x92;
}
////////////////////////////////////IO初始化//////////////////////////////////
void InitIO (void)
{
	P1M0=0x00;
	P1M1=0x00;
	P2M0=0x0f;
	P2M1=0x00;
	P3M0=0x00;
	P3M1=0x00;
	P4M0=0x00;
	P4M1=0x00;
	P2 = 0;
}
void chek(void)
{
	if( Key_C02!=0 )
	{
		delayms(10);
		if( Key_C02!=0 )
		{
			 flag_C02 = 1;
		}
	}
}
/////////////////////////////////main函数//////////////////////////////////////
int  main(void)
{
	InitCPU();
 	InitIO();
	Motor_Stop();
	chek();
	while(1)
	{
		GetKey();	
		run();
	}
	return 0;	
}
//////////////////////////////////////中断/////////////////////////////////
void timer0_int() interrupt 1
{ 
	cont++;                                      //250us中断
	if( cont>=4 )
	{
		delay_1ms++;
		delay_ms++;
		delay_1ms_for_100ms++;
		cont = 0;
	}
	if(delay_ms>60000)
		delay_ms = 0;
	if( delay_1ms_for_100ms>=100 )
	{
		delay_1ms_for_100ms = 0;
		read_C02();
		
	}
	if( delay_1ms>= 1000)
	{
		delay_1s ++;
		delay_1ms = 0;

		if( L1_runtime !=0 )
			L1_runtime--;

		if( L2_runtime !=0 )
			L2_runtime--;

		
// 		if( Key_Frame_21_time_flag==1 )
// 			Key_Frame_21_time++;
		
		if( Key_Frame_23_time_flag==1 )
			Key_Frame_23_time++;
		
		if( L1_delay_flag==1 )
		{
			MOTOR_L1_Runtime ++;
		}
		else
		{
			MOTOR_L1_Runtime = 0;
		}
	}	
	if( delay_1s> 300 )
		delay_1s = 0;						  //防止自加超出
		
}