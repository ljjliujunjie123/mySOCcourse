	/*
	 * ʹ��ϵͳĬ����Ƶ1.05MHz
	 * SMCLKʱ��Դ����������ģʽ
	 * ����S3-S6����ѡ��բ��
	 * ����S7���ڿ�ʼƵ�ʼ���
	 * ���а�����բ�Ź�ϵ��Ӧ���£�
	 * S3----->0.01s
	 * S4----->0.1s
	 * S5----->1s
	 * S6----->10s
	 * բ��ѡ��ͨ��Һ����ʾ��Ƶ�ʼ��������Һ����ʾ
	 * ��ʾ��λHz
	 */
#include <msp430f6638.h>
#include "dr_lcdseg.h"
#define SW3	BIT4 /*�����궨��*/
#define SW4	BIT3
#define SW5	BIT2
#define SW6	BIT1
#define SW7	BIT0
char gTimer_count =0;//Ϊ��ʱ�䶨ʱ����
int gGate=10;
long gFCount=0;
void CymometerComplete(long fc,int gate)
{//��ʾ��ǰ����Ƶ��
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
	default: break;//�ݲ�֧������բ�ų�ʼ��
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
		TA0CTL&=~MC_3;//������ֹͣ
		P1IE&=~BIT3;//�ر�P1�ж�
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
	P4REN 	|= 0X1F;//ʹ�����õ���
	P4OUT		|= 0X1F;//����Ϊ��������
	P4IFG		&=~0X1F;
	P4IE			|= 0X1F;
}
#pragma vector=PORT4_VECTOR
__interrupt void Port4Interrupt(void)
{//������Ӧ
	if(P4IFG&0X1F)
	{
		switch(P4IFG)
		{
		case SW3:TimerA0_init(10);			break;
		case SW4:TimerA0_init(100);		break;
		case SW5:TimerA0_init(1000);		break;
		case SW6:TimerA0_init(10000);	break;
		case SW7://��ʼ��Ƶ
				gFCount=0;
				TA0CTL|=TACLR;
				TA0CTL |=MC_1;
				P1IE=BIT3;
		default:break;//�������ͬʱ������������
		}
	}
	P4IFG=0;
}
#pragma vector=PORT1_VECTOR
__interrupt void Port1Interrupt(void)
{//��P1.3�����ж�������
	if(P1IFG&BIT3)
	{
		gFCount++;
	}
	P1IFG=0;
}
void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;
	TA0CTL = TASSEL_2+MC_0+TACLR;//Ĭ�϶�ʱ������
	TA0CCR0 = 10450;
	TA0CCTL0|=CCIE;
	gGate=10;
	initLcdSeg();
	ShowCymometerGate(gGate);
	GPIO_init();
	__bis_SR_register(LPM0_bits+GIE);
}
