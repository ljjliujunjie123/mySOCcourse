#include <msp430f6638.h>
#include <stdio.h>
#include "dr_lcdseg.h"
#include <stdint.h>
typedef struct                   //��ָ����ʽ����P8�ڵĸ����Ĵ���
{
  const volatile uint8_t* PxIN;     //����һ�����ᱻ������޷����ַ��ͱ���
  volatile uint8_t* PxOUT;
  volatile uint8_t* PxDIR;
  volatile uint8_t* PxREN;
  volatile uint8_t* PxSEL;
} GPIO_TypeDef;

const GPIO_TypeDef GPIO4 =
{ &P4IN, &P4OUT, &P4DIR, &P4REN, &P4SEL};

const GPIO_TypeDef GPIO5 =
{&P5IN, &P5OUT, &P5DIR, &P5REN, &P5SEL};

const GPIO_TypeDef GPIO8 =
{&P8IN, &P8OUT, &P8DIR, &P8REN, &P8SEL};

const GPIO_TypeDef* LED_GPIO[5] = {&GPIO4, &GPIO4, &GPIO4, &GPIO5, &GPIO8};
const uint8_t LED_PORT[5] = {BIT5, BIT6, BIT7, BIT7, BIT0};

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

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  while(BAKCTL & LOCKIO) // Unlock XT1 pins for operation
    BAKCTL &= ~(LOCKIO);
  UCSCTL6 &= ~XT1OFF; //����XT1
  while (UCSCTL7 & XT1LFOFFG) //�ȴ�XT1�ȶ�
    UCSCTL7 &= ~(XT1LFOFFG);

  UCSCTL4 = SELA__XT1CLK + SELS__REFOCLK + SELM__REFOCLK; //ʱ����ΪXT1��Ƶ�ʽϵͣ����������ʱ

  int i;
  for(i=0;i<5;++i)
    *LED_GPIO[i]->PxDIR |= LED_PORT[i]; //���ø�LED�����ڶ˿�Ϊ�������

  P4REN |= 0x1F; //ʹ�ܰ����˿��ϵ�����������
  P4OUT |= 0x1F; //����״̬

  initClock();             //����ϵͳʱ��
  initLcdSeg();           //��ʼ����ʽҺ��

  uint8_t last_btn = 0x1F, cur_btn, temp;
  int syncflag = 1,pressState = 0;
  while(1)
  {
    cur_btn = P4IN & 0x1F;
    temp = (cur_btn ^ last_btn) & last_btn; //�ҳ�����������İ���
    last_btn = cur_btn;
    int i=0,j=0,k=0;
    for(i=0;i<5;++i)
      if(temp & (1 << i))
        pressState ^= 1;
    __delay_cycles(3276); //��ʱ��Լ100ms*/

    for(i=0;i<4;i++)
        *LED_GPIO[i]->PxOUT &=~ LED_PORT[i];

    if(!pressState){
    	if(syncflag){
    		for(j=0;j<2;j++)
    			*LED_GPIO[j]->PxOUT ^= LED_PORT[j];
    	}
    	else{
    		for(k=2;k<4;k++)
    			*LED_GPIO[k]->PxOUT ^= LED_PORT[k];
    	}
    	for(i=3;i>0;i--){
    		LCDSEG_SetDigit(1,i);
    		__delay_cycles(3276000);
    		LCDSEG_SetDigit(1,-1);
    		__delay_cycles(3276000);
    	}
    }else{
    	for(i=0;i<4;i++)
    		*LED_GPIO[i]->PxOUT |= LED_PORT[i];
    }

	syncflag ^= 1;
  }
}


