#include <msp430.h>
#include <stdint.h>
#include "dr_lcdseg.h"

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
  0x40, //display -
};

void initLcdSeg()
{
  //端口设定
  P5SEL |= BIT3 + BIT4 + BIT5; //P5.3 .4 .5作为LCD的COM
  LCDBPCTL0 = 0x0FFF; //S0~S11所在端口作为LCD的段选
  //控制器设定
  LCDBCTL0 = LCDDIV_21 + LCDPRE__4 + LCD4MUX; //ACLK, 21*4分频，合48.76Hz
  LCDBMEMCTL |= LCDCLRM; //清空LCD存储器
  LCDBCTL0 |= LCDSON + LCDON; //启动LCD模块
}

void LCDSEG_SetDigit(int pos, int value) //value不在0~16时熄灭,16显示-
{
  if(pos < 0 || pos > 6)
    return;
  pos = 5 - pos;
  
  uint8_t temp, mem;
  if(value < 0 || value > 16)
    temp = 0x00;
  else
    temp = SEG_CTRL_BIN[value];
 
  const static uint8_t map[7] = 
  { BIT7, BIT6, BIT5, BIT0, BIT1, BIT3, BIT2 };
  
  mem = LCDMEM[pos];
  mem &= 0x10; //清空控制数字段的位
  int i;
  for(i=0;i<7;++i)
  {
    if(temp & (1 << i))
      mem |= map[i];
  }
  LCDMEM[pos] = mem;
}

void LCDSEG_SetSpecSymbol(int pos)
{
  LCDMEM[pos] |= 0x10;
}

void LCDSEG_ResetSpecSymbol(int pos)
{
  LCDMEM[pos] &= ~0x10;
}

void LCDSEG_DisplayNumber(int32_t num, int dppos)
{
  int curpos = 0, isneg = 0;
  
  if(num < 0)
  {
    isneg = 1; 
    num = -num;
  }
  
  while(1)
  {
    int digit = num % 10;
    num /= 10;
    LCDSEG_SetDigit(curpos++, digit);
    if(num == 0)
      break;
  }
  
  if(isneg)
    LCDSEG_SetDigit(curpos++, 16); //加负号
  
  while(curpos < 6)
    LCDSEG_SetDigit(curpos++, -1); //将多余位清空
  
  int i;
  for(i=3;i<=5;++i)
    LCDSEG_ResetSpecSymbol(i);
  
  if(dppos > 0 && dppos <= 3)
    LCDSEG_SetSpecSymbol(6 - dppos);
}
