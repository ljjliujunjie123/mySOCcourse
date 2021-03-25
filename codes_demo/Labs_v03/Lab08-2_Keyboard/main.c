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

int n=0,cs=0;
void Led_Display(long number);

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

const int KEYBOARD_VALUE[16] =
{
  15, 14, 13, 12, 11, 10, 0, 9,
  8, 7, 6, 5, 4, 3, 2, 1
};

uint8_t seg_display = 0, kb_line = 0;
uint16_t cur_kb_input = 0xFFFF, last_kb_input = 0xFFFF;
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

int kb_in; //保存检查键盘输入用的I2C查询代号

#pragma vector=TIMER2_A0_VECTOR
__interrupt void softTimerInterrupt()
{
  static int kbcount = 0;

  kbcount++;
  I2C_CheckQuery(kb_in); //空读取，迫使I2C模块更新数据
  if(kbcount > 10)
  {
    kbcount = 0;

    cur_kb_input &= ~(0xF << (kb_line * 4) );
	cur_kb_input |= ((I2C_CheckQuery(kb_in) & 0xF) << (kb_line * 4));

	uint16_t temp = (cur_kb_input ^ last_kb_input) & last_kb_input; //找出向下跳变的按键
	int i;
	for(i=0;i<16;++i)
	{
          if(temp & (1<<i))
          {
        	  if(i>=12&&i<=15)	{P5OUT&=~BIT7;P8OUT&=~BIT0;}
        	  if(i>=8&&i<=11)	{P5OUT|=BIT7;P8OUT&=~BIT0;}
        	  if(i>=4&&i<=7)	{P5OUT&=~BIT7;P8OUT|=BIT0;}
        	  if(i>=0&&i<=3)	{P5OUT|=BIT7;P8OUT|=BIT0;}
        	  break;
          }
	}
	last_kb_input = cur_kb_input;

	kb_line++;
	if(kb_line >= 4)
	kb_line = 0;

	I2C_RequestSend(KB_ADDR, KB_OUT, ~(1 << (kb_line + 4)) );
  }
}

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  _DINT();
  initClock();
  initI2C();
  _EINT();

  P5SEL&=~BIT7;
  P5DIR|=BIT7;
  P5OUT&-~BIT7;
  P8SEL&=~BIT0;
  P8DIR|=BIT0;
  P8OUT&-~BIT0;

  //设置矩阵键盘的IO扩展器端口方向
  I2C_RequestSend(KB_ADDR, KB_DIR, 0x0F);
  //设置矩阵键盘的初始扫描线
  I2C_RequestSend(KB_ADDR, KB_OUT, ~(1 << (kb_line + 4)) );
  //要求自动查询键盘输入
  kb_in = I2C_AddRegQuery(KB_ADDR, KB_IN);

    while(1);
}
