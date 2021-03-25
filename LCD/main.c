#include <msp430f6638.h>
#include <stdio.h>
#include "dr_lcdseg.h"
#include <string.h>
#include <stdint.h>

#define XT2_FREQ   4000000
#define MCLK_FREQ 16000000
#define SMCLK_FREQ 4000000

void initClock()
{
  while(BAKCTL & LOCKIO) //����XT1���Ų���
BAKCTL &= ~(LOCKIO);
UCSCTL6 &= ~XT1OFF; //����XT1��ѡ���ڲ�ʱ��Դ
    P7SEL |= BIT2 + BIT3; //XT2���Ź���ѡ��
UCSCTL6 &= ~XT2OFF; //����XT2
while (SFRIFG1 & OFIFG) //�ȴ�XT1��XT2��DCO�ȶ�
{
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
    }
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__XT2CLK; //����DCO�������ܷ�
  UCSCTL1 = DCORSEL_5; //6000kHz~23.7MHz
  UCSCTL2 = MCLK_FREQ / (XT2_FREQ / 16); //XT2Ƶ�ʽϸߣ���Ƶ����Ϊ��׼�ɻ�ø��ߵľ���
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2����16��Ƶ����Ϊ��׼
  while (SFRIFG1 & OFIFG) //�ȴ�XT1��XT2��DCO�ȶ�
{
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //�趨����CLK�ķ�Ƶ
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__DCOCLK; //�趨����CLK��ʱ��Դ
}

void showSomething(int *data){
	int i;
	for(i=0;i<6;i++){
		LCDSEG_SetDigit(5-i,data[i]);
	}
}

void clearLCD(void){
	int i;
	for(i=0;i<6;i++)LCDSEG_SetDigit(i,-1);
}

void main(void)
{
	unsigned char i,num1;
	int32_t num2;
    WDTCTL = WDTPW | WDTHOLD;	// ֹͣ���Ź�
    initClock();             //����ϵͳʱ��
    initLcdSeg();           //��ʼ����ʽҺ��

    int classID[6] = {0x0F,1,8,0,8,2};
    int time[6] = {2,1,0,3,2,2};
    while(1)               //���������ѭ��
    {
    	showSomething(classID);
    	__delay_cycles(MCLK_FREQ); //��ʱ1000ms
    	clearLCD();
    	showSomething(time);
    	__delay_cycles(MCLK_FREQ); //��ʱ1000ms
    	clearLCD();
    }
}