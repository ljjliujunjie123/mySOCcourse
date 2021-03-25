/*
 * main.c
 */
#include <msp430.h>
#include <stdint.h>
#include "dr_i2c.h"

#define SEG_ADDR 0x20
#define SEG_CS 2
#define SEG_SS 3
#define SEG_DIR0 6
#define SEG_DIR1 7
#define KB_ADDR 0x21
#define KB_IN 0
#define KB_OUT 1
#define KB_PIR 2
#define KB_DIR 3

const uint8_t SEG_CTRL_BIN[17] =
{
  0x3F,	//display 0
  0x06,	//display 1
  0x5B,	//display 2
  0x4F,	//display 3
  0x66,	//display 4
  0x6D,	//display 5
  0x7D,	//display 6
  0x07,	//display 7
  0x7F,	//display 8
  0x6F,	//display 9
  0x77,	//display A
  0x7C,	//display b
  0x39,	//display C
  0x5E,	//display d
  0x79, //display E
  0x71, //display F
  0x80, //display .
};

const int KEYBOARD_VALUE[16] =       //定义判别16个按键的数组
{
  15, 14, 13, 12, 11, 10, 0, 9,
  8, 7, 6, 5, 4, 3, 2, 1
};

uint8_t seg_display = 0;      //定义其他一些标识数
uint8_t seg_buffer[8] =
{
  0x3F,	0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07
};

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
  UCSCTL2 = 16000000 / (4000000 / 16); //XT2频率较高，分频后作为基准可获得更高的精度
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2进行16分频后作为基准
  while (SFRIFG1 & OFIFG) { //等待XT1、XT2与DCO稳定
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //设定几个CLK的分频
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__DCOCLK; //设定几个CLK的时钟源

  TA2CTL = TASSEL__SMCLK + MC__UP + ID__1;
  TA2CCR0 = (4000000 / 1000) - 1;
  TA2CCTL0 = CCIE;
}


#pragma vector=TIMER2_A0_VECTOR
__interrupt void softTimerInterrupt()
{
  static int cs = 0;
  I2C_RequestSend(SEG_ADDR, SEG_SS, 0);
  I2C_RequestSend(SEG_ADDR, SEG_CS, 1 << cs);
  I2C_RequestSend(SEG_ADDR, SEG_SS, seg_buffer[cs]);	//点亮第cs位数码管
  cs++;
  if(cs >= 8)					//如果达到第八位则重新回到第一位
    cs = 0;

  static int count = 0;

  count++;
  if(count > 1000)
  {
    count = 0;

    seg_display++;
    if(seg_display >= 16)
      seg_display = 0;
  }
}

int main( void )
{
  WDTCTL = WDTPW + WDTHOLD;		//关闭看门狗

  _DINT();						//关闭总中断
  initClock();					//配置系统时钟
  initI2C();					//初始化I2C
  _EINT();						//开启总中断

  //设置数码管的IO扩展器端口方向
  I2C_RequestSend(SEG_ADDR, SEG_DIR0, 0);
  I2C_RequestSend(SEG_ADDR, SEG_DIR1, 0);

  while(1)
  {
    int i, j;		//改变数码管的标识位使得数码管显示的数字变化显示
    for(i=seg_display, j=0;j<8;++j)
    {
      seg_buffer[j] = SEG_CTRL_BIN[i];
      i++;
      if(i >= 16)
        i = 0;
    }
  }
}
