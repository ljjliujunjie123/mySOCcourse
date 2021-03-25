	/*
	 * 使用系统默认主频1.05MHz
	 * SMCLK时钟源，继续计数模式
	 * 按键S3-S6用于选择闸门
	 * 按键S7用于开始频率计数
	 * 其中按键与闸门关系对应如下：
	 * S3----->0.01s
	 * S4----->0.1s
	 * S5----->1s
	 * S6----->10s
	 * 闸门选择通过液晶显示，频率计数结果，液晶显示
	 * 显示单位Hz
	 */
#include <msp430f6638.h>
#include "dr_lcdseg.h"
#define SW3	BIT4 /*按键宏定义*/
#define SW4	BIT3
#define SW5	BIT2
#define SW6	BIT1
#define SW7	BIT0
char gTimer_count =0;//为长时间定时辅助
int gGate=10;
long gFCount=0;
void CymometerComplete(long fc,int gate)
{//显示当前计数频率
	switch(gate)
	{
	case 10:		fc*=100;break;
	case 100:		fc*=10;break;
	case 10000:fc/=10;break;
	default:break;
	}
	LCDSEG_DisplayNumber(fc,0);
}
void ShowCymometerGate(int gate)
{
	char i=0,gateTemp=0;
	for(;i<6;i++)
		LCDSEG_SetDigit(i,-1);
	for(i=0;gate;gate/=10)
	{
		gateTemp=gate%10;
		LCDSEG_SetDigit(i,gateTemp);
		i++;
	}
	LCDSEG_SetDigit(5,10);
}
void TimerA0_init(int ms)
{
	switch(ms)
	{
	case 10		:
		TA0CTL = TASSEL_2+MC_0+TACLR;
		TA0CCR0 = 10450;
		TA0CCTL0|=CCIE;
		gGate=10;
		break;
	case 100		:
		TA0CTL = TASSEL_2+MC_0+ID_1+TACLR;
		TA0CCR0 = 52250;
		TA0CCTL0|=CCIE;
		gGate=100;
		break;
	case 1000	:
		TA0CTL = TASSEL_2+MC_0+ID_3+TACLR;
		gTimer_count=2;
		TA0CCR0 = 65313;
		TA0CCTL0|=CCIE;
		gGate=1000;
		break;
	case 10000	:
		TA0CTL = TASSEL_2+MC_0+ID_3+TACLR;
		gTimer_count=20;
		TA0CCR0 = 65313;
		TA0CCTL0|=CCIE;
		gGate=10000;
		break;
	default: break;//暂不支持其他闸门初始化
	}
	ShowCymometerGate(gGate);
}
#pragma vector = TIMER0_A0_VECTOR
__interrupt void TimerA0_ISR(void)
{
	if(gTimer_count!=0)
	{
		gTimer_count--;
	}
	if(gTimer_count==0)
	{
		TA0CTL&=~MC_3;//计数器停止
		P1IE&=~BIT3;//关闭P1中断
		CymometerComplete(gFCount,gGate);
	}
}
void GPIO_init(void)
{
	P1DIR		&=~BIT3;
	P1IFG		=0;
	P1IES		|=BIT3;
	P1REN 	|=BIT3;
	P1OUT		|=BIT3;
	P4DIR 		= 0;
	P4IES 		|= 0X1F;
	P4REN 	|= 0X1F;//使能内置电阻
	P4OUT		|= 0X1F;//并设为上拉电阻
	P4IFG		&=~0X1F;
	P4IE			|= 0X1F;
}
#pragma vector=PORT4_VECTOR
__interrupt void Port4Interrupt(void)
{//按键响应
	if(P4IFG&0X1F)
	{
		switch(P4IFG)
		{
		case SW3:TimerA0_init(10);			break;
		case SW4:TimerA0_init(100);		break;
		case SW5:TimerA0_init(1000);		break;
		case SW6:TimerA0_init(10000);	break;
		case SW7://开始计频
				gFCount=0;
				TA0CTL|=TACLR;
				TA0CTL |=MC_1;
				P1IE=BIT3;
		default:break;//多个按键同时触发不做处理
		}
	}
	P4IFG=0;
}
#pragma vector=PORT1_VECTOR
__interrupt void Port1Interrupt(void)
{//由P1.3输入中断来计数
	if(P1IFG&BIT3)
	{
		gFCount++;
	}
	P1IFG=0;
}
void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;
	TA0CTL = TASSEL_2+MC_0+TACLR;//默认定时器设置
	TA0CCR0 = 10450;
	TA0CCTL0|=CCIE;
	gGate=10;
	initLcdSeg();
	ShowCymometerGate(gGate);
	GPIO_init();
	__bis_SR_register(LPM0_bits+GIE);
}
