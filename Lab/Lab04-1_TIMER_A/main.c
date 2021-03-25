/*
 * main.c
 */
#include <msp430f6638.h>

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;//关闭看门狗
	P1DIR |= BIT5;//控制蜂鸣器输出
	P4DIR |= BIT5;//控制LED输出
	TA0CTL |= MC_1 + TASSEL_2 + TACLR;
	//时钟为SMCLK,比较模式，开始时清零计数器
	TA0CCTL0 = CCIE;//比较器中断使能
	TA0CCR0  = 50000;//比较值设为50000，相当于50ms的时间间隔
	__bis_SR_register(LPM0_bits + GIE);//进入低功耗并开启总中断
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	P1OUT ^= BIT5;//形成鸣叫效果
	P4OUT ^= BIT5;//形成闪灯效果
}
