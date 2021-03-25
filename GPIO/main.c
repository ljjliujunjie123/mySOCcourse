#include <msp430f6638.h>
#include <stdio.h>
#include "dr_lcdseg.h"
#include <stdint.h>
typedef struct                   //以指针形式定义P8口的各个寄存器
{
  const volatile uint8_t* PxIN;     //定义一个不会被编译的无符号字符型变量
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
  while(BAKCTL & LOCKIO) //解锁XT1引脚操作
BAKCTL &= ~(LOCKIO);
UCSCTL6 &= ~XT1OFF; //启动XT1，选择内部时钟源
    P7SEL |= BIT2 + BIT3; //XT2引脚功能选择
UCSCTL6 &= ~XT2OFF; //启动XT2
while (SFRIFG1 & OFIFG) //等待XT1、XT2与DCO稳定
{
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
    }
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__XT2CLK; //避免DCO调整中跑飞
  UCSCTL1 = DCORSEL_5; //6000kHz~23.7MHz
  UCSCTL2 = MCLK_FREQ / (XT2_FREQ / 16); //XT2频率较高，分频后作为基准可获得更高的精度
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2进行16分频后作为基准
  while (SFRIFG1 & OFIFG) //等待XT1、XT2与DCO稳定
{
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //设定几个CLK的分频
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__DCOCLK; //设定几个CLK的时钟源
}

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  while(BAKCTL & LOCKIO) // Unlock XT1 pins for operation
    BAKCTL &= ~(LOCKIO);
  UCSCTL6 &= ~XT1OFF; //启动XT1
  while (UCSCTL7 & XT1LFOFFG) //等待XT1稳定
    UCSCTL7 &= ~(XT1LFOFFG);

  UCSCTL4 = SELA__XT1CLK + SELS__REFOCLK + SELM__REFOCLK; //时钟设为XT1，频率较低，方便软件延时

  int i;
  for(i=0;i<5;++i)
    *LED_GPIO[i]->PxDIR |= LED_PORT[i]; //设置各LED灯所在端口为输出方向

  P4REN |= 0x1F; //使能按键端口上的上下拉电阻
  P4OUT |= 0x1F; //上拉状态

  initClock();             //配置系统时钟
  initLcdSeg();           //初始化段式液晶

  uint8_t last_btn = 0x1F, cur_btn, temp;
  int syncflag = 1,pressState = 0;
  while(1)
  {
    cur_btn = P4IN & 0x1F;
    temp = (cur_btn ^ last_btn) & last_btn; //找出刚向下跳变的按键
    last_btn = cur_btn;
    int i=0,j=0,k=0;
    for(i=0;i<5;++i)
      if(temp & (1 << i))
        pressState ^= 1;
    __delay_cycles(3276); //延时大约100ms*/

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


