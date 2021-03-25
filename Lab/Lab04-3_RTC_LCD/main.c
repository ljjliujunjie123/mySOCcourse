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
  UCSCTL6 &= ~XT1OFF; //启动XT1
  P7SEL |= BIT2 + BIT3; //XT2引脚功能选择
  UCSCTL6 &= ~XT2OFF; //启动XT2
  while (SFRIFG1 & OFIFG) { //等待XT1、XT2与DCO稳定
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }

  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__XT2CLK; //避免DCO调整中跑飞

  UCSCTL1 = DCORSEL_5; //6000kHz~23.7MHz
  UCSCTL2 = MCLK_FREQ / (XT2_FREQ / 16); //XT2频率较高，分频后作为基准可获得更高的精度
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2进行16分频后作为基准
  while (SFRIFG1 & OFIFG) { //等待XT1、XT2与DCO稳定
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //设定几个CLK的分频
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__DCOCLK; //设定几个CLK的时钟源
}

void SetupRTC(void)
{
    RTCCTL01 |= RTCBCD + RTCHOLD;   //数据格式为BCD码
    RTCHOUR = 0x00;
    RTCMIN = 0x00;
    RTCSEC = 0x00;
    RTCCTL0 |= RTCRDYIE;            // RTCRDY中断使能
}

void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    initClock();
    initLcdSeg();
    SetupRTC();    //初始化RTC
    P4DIR &=~0x1F;
    P4REN |= 0x1F; //使能按键端口上的上下拉电阻
    P4OUT |= 0x1F; //上拉状态
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
            hourBCD =  (RTCHOUR>>4)*10+(RTCHOUR&0x0f); //获取小时数据
               minuteBCD = (RTCMIN>>4)*10+(RTCMIN&0x0f);  //获取分钟数据
               secondBCD = (RTCSEC>>4)*10+(RTCSEC&0x0f);  //获取秒数据
               show=hourBCD*10000+minuteBCD*100+secondBCD;//变换为显示数据
               if(minuteBCD>0)
               	LCDSEG_DisplayNumber(show, 2); //显示（有分钟时显示小数点）
               else
               	LCDSEG_DisplayNumber(show, 0);//显示（无小数点）
        	break;
        case RTC_RTCTEVIFG:break;
        case RTC_RTCAIFG: break;
        case RTC_RT0PSIFG:break;  //分频器 0
        case RTC_RT1PSIFG: break; // 分频器 1
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

