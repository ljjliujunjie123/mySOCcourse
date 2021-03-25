/*
 * main.c
 */
#include <msp430f6638.h>

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 //关闭看门狗
  P4DIR |= BIT5;                            //设置P4.5口方向为输出
  P4DIR &= ~BIT0;
  P4REN |= BIT0;                            //使能P4.0上拉电阻
  P4OUT |= BIT0;                            //P4.0口置高电平
  P4IES |= BIT0;                            //中断沿设置（下降沿触发）
  P4IFG &= ~BIT0;                           //清P4.0中断标志
  P4IE |= BIT0;                             //使能P4.0口中断

  __bis_SR_register(LPM4_bits + GIE);       //进入低功耗模式4 开中断

  __no_operation();                         //空操作
}

// P4中断函数
#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
{
  __delay_cycles(10000);
  if ((P4IN & BIT0)==0){
      P4OUT ^= BIT5;                       //改变LED5灯状态
  }
  P4IFG &= ~BIT0;                          //清P4.0中断标志位
}
