/*
 * main.c
 */
#include <msp430.h>

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

  uint8_t last_btn = 0x1F, cur_btn, temp;
  while(1)
  {
    cur_btn = P4IN & 0x1F;
    temp = (cur_btn ^ last_btn) & last_btn; //找出刚向下跳变的按键
    last_btn = cur_btn;
    int i;
    for(i=0;i<5;++i)
      if(temp & (1 << i))
        *LED_GPIO[i]->PxOUT ^= LED_PORT[i]; //翻转对应的LED
    __delay_cycles(3276); //延时大约100ms
  }
}
