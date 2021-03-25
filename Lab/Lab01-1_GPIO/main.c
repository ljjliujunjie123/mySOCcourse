/*
 * main.c
 */
#include <msp430f6638.h>
void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;    // 关闭看门狗
  P4DIR |= BIT5;                // 设置4.5口为输出模式
  P4OUT |= BIT0;				// 选中P4.0为输出方式
  P4REN |= BIT0;				// P4.0使能

  while (1)                   // Test P1.4
  {
    if (P4IN & BIT0)		//如果P4.0为1则执行，这是查询方式按下去后是低，否则为高
    	P4OUT |= BIT5;     //使P4.5置高
    else
     P4OUT &= ~BIT5;     // else reset
  }
}
