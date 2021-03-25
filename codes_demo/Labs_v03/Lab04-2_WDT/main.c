/*
 * main.c
 */
#include<msp430f6638.h>

void main(void)
{
	WDTCTL = WDT_ADLY_1000; //#define WDT_ADLY_1000 (WDTPW+WDTTMSEL+WDTCNTCL+WDTIS2+WDTSSEL0)
						  // 时钟为ACLK, 模式为内部时钟。
	SFRIE1 |= WDTIE;      // 开启看门狗中断
	P4DIR |= BIT5;
	__bis_SR_register(LPM0_bits + GIE); // 进入低功耗，开启总中断
}

#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR(void)
{
	P4OUT ^= BIT5;        // P4.5口取反
}
