/*
 * main.c
 */
#include<msp430f6638.h>

void main(void)
{
	WDTCTL = WDT_ADLY_1000; //#define WDT_ADLY_1000 (WDTPW+WDTTMSEL+WDTCNTCL+WDTIS2+WDTSSEL0)
						  // ʱ��ΪACLK, ģʽΪ�ڲ�ʱ�ӡ�
	SFRIE1 |= WDTIE;      // �������Ź��ж�
	P4DIR |= BIT5;
	__bis_SR_register(LPM0_bits + GIE); // ����͹��ģ��������ж�
}

#pragma vector=WDT_VECTOR
__interrupt void WDT_ISR(void)
{
	P4OUT ^= BIT5;        // P4.5��ȡ��
}
