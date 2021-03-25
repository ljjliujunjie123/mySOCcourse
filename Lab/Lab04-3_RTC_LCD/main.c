#include <msp430f6638.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "dr_lcdseg.h"
#define XT2_FREQ   4000000
#define MCLK_FREQ 16000000
#define SMCLK_FREQ 4000000
typedef struct
{
  const volatile uint8_t* PxIN;
  volatile uint8_t* PxOUT;
  volatile uint8_t* PxDIR;
  volatile uint8_t* PxREN;
  volatile uint8_t* PxSEL;
} GPIO_TypeDef;

const GPIO_TypeDef GPIO4 =
{
  &P4IN, &P4OUT, &P4DIR, &P4REN, &P4SEL
};
unsigned int hourBCD=0;
unsigned int minuteBCD=0;
unsigned int secondBCD=0;
unsigned int show=0;
void initClock()
{
  while(BAKCTL & LOCKIO) // Unlock XT1 pins for operation
    BAKCTL &= ~(LOCKIO);
  UCSCTL6 &= ~XT1OFF; //����XT1
  P7SEL |= BIT2 + BIT3; //XT2���Ź���ѡ��
  UCSCTL6 &= ~XT2OFF; //����XT2
  while (SFRIFG1 & OFIFG) { //�ȴ�XT1��XT2��DCO�ȶ�
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }

  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__XT2CLK; //����DCO�������ܷ�

  UCSCTL1 = DCORSEL_5; //6000kHz~23.7MHz
  UCSCTL2 = MCLK_FREQ / (XT2_FREQ / 16); //XT2Ƶ�ʽϸߣ���Ƶ����Ϊ��׼�ɻ�ø��ߵľ���
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2����16��Ƶ����Ϊ��׼
  while (SFRIFG1 & OFIFG) { //�ȴ�XT1��XT2��DCO�ȶ�
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //�趨����CLK�ķ�Ƶ
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__DCOCLK; //�趨����CLK��ʱ��Դ
}

void SetupRTC(void)
{
    RTCCTL01 |= RTCBCD + RTCHOLD;   //���ݸ�ʽΪBCD��
    RTCHOUR = 0x00;
    RTCMIN = 0x00;
    RTCSEC = 0x00;
    RTCCTL0 |= RTCRDYIE;            // RTCRDY�ж�ʹ��
}

void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    initClock();
    initLcdSeg();
    SetupRTC();    //��ʼ��RTC
    P4DIR &=~0x1F;
    P4REN |= 0x1F; //ʹ�ܰ����˿��ϵ�����������
    P4OUT |= 0x1F; //����״̬
    P4IE |= 0xFF;
    P1IES |= 0x1F;
    P1IFG &= ~0x1F;
    _EINT();
    LCDSEG_DisplayNumber(show, 0);
    LPM3;
}

#pragma vector=RTC_VECTOR
__interrupt void RTC_ISR(void)
{
    switch (__even_in_range(RTCIV, RTC_RT1PSIFG))
    {
        case RTC_NONE: break;
        case RTC_RTCRDYIFG:
            hourBCD =  (RTCHOUR>>4)*10+(RTCHOUR&0x0f); //��ȡСʱ����
               minuteBCD = (RTCMIN>>4)*10+(RTCMIN&0x0f);  //��ȡ��������
               secondBCD = (RTCSEC>>4)*10+(RTCSEC&0x0f);  //��ȡ������
               show=hourBCD*10000+minuteBCD*100+secondBCD;//�任Ϊ��ʾ����
               if(minuteBCD>0)
               	LCDSEG_DisplayNumber(show, 2); //��ʾ���з���ʱ��ʾС���㣩
               else
               	LCDSEG_DisplayNumber(show, 0);//��ʾ����С���㣩
        	break;
        case RTC_RTCTEVIFG:break;
        case RTC_RTCAIFG: break;
        case RTC_RT0PSIFG:break;  //��Ƶ�� 0
        case RTC_RT1PSIFG: break; // ��Ƶ�� 1
        default: break;
    }
    __no_operation();
}

#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
{if(P4IFG&0x1F)
	{switch(P4IFG&0x1F)
    	{case 0x01:     //S7
    	    RTCCTL01 &= ~RTCHOLD;
    	    break;
    	case 0x02:     //S6
    		break;
    	case 0x04:     //S5
    		RTCCTL01 |= RTCHOLD;
    		break;
    	case 0x08:     //S4
    		break;
    	case 0x10:     //S3
    	    RTCCTL01 |= RTCHOLD;
    	    RTCHOUR = 0x00;
    	    RTCMIN = 0x00;
    	    RTCSEC = 0x00;
    	    show=0;
    	    LCDSEG_DisplayNumber(show, 0);
    	    break;
    	default:
    		break;
    	}
    	P4IFG&= ~0x1F;
	}
}

