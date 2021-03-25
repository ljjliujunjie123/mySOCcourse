/*
 * main.c
 */
#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include "dr_tft.h"


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
  UCSCTL2 = 20000000 / (4000000 / 16); //XT2频率较高，分频后作为基准可获得更高的精度
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2进行16分频后作为基准
  while (SFRIFG1 & OFIFG) { //等待XT1、XT2与DCO稳定
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //设定几个CLK的分频
  UCSCTL4 = SELA__XT1CLK + SELS__DCOCLK + SELM__DCOCLK; //设定几个CLK的时钟源
}

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  _DINT();

  initClock();
  initTFT();

  _EINT();

  etft_AreaSet(0,0,319,239,0);


  while(1)
  {
	  etft_AreaSet(0,0,39,239,0);
	  etft_AreaSet(40,0,79,239,31);
	 etft_AreaSet(80,0,119,239,2016);
	  etft_AreaSet(120,0,159,239,63488);
	 etft_AreaSet(160,0,199,239,2047);
	 etft_AreaSet(200,0,239,239,63519);
	  etft_AreaSet(240,0,279,239,65504);
	  etft_AreaSet(280,0,319,239,65535);
	  __delay_cycles(MCLK_FREQ*3);
	  etft_AreaSet(0,0,319,239,0);
	  __delay_cycles(MCLK_FREQ);
	  etft_DisplayString("TI MSP430F6638 EVM",100,80,65535,0);
	  etft_DisplayString("TI UNIVERSITY PROGRAM",0,150,63488,0);
	  etft_DisplayString("- TSINGHUA UNIVERSITY",100,180,65504,0);
	  __delay_cycles(MCLK_FREQ*3);


  }
}
