/*
 * main.c
 */
#include <msp430f6638.h>

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;//�رտ��Ź�
	P1DIR |= BIT5;//���Ʒ��������
	P4DIR |= BIT5;//����LED���
	TA0CTL |= MC_1 + TASSEL_2 + TACLR;
	//ʱ��ΪSMCLK,�Ƚ�ģʽ����ʼʱ���������
	TA0CCTL0 = CCIE;//�Ƚ����ж�ʹ��
	TA0CCR0  = 50000;//�Ƚ�ֵ��Ϊ50000���൱��50ms��ʱ����
	__bis_SR_register(LPM0_bits + GIE);//����͹��Ĳ��������ж�
}

#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A (void)
{
	P1OUT ^= BIT5;//�γ�����Ч��
	P4OUT ^= BIT5;//�γ�����Ч��
}
