/*
 * main.c
 */
#include <msp430f6638.h>
int flag = 0;

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 //�رտ��Ź�
  P4DIR |= BIT5;                            //����P4.5�ڷ���Ϊ���
  P4DIR &= ~BIT0;
  P4REN |= BIT0;                            //ʹ��P4.0��������
  P4OUT |= BIT0;                            //P4.0���øߵ�ƽ
  P4IES |= BIT0;                            //�ж������ã��½��ش�����
  P4IFG &= ~BIT0;                           //��P4.0�жϱ�־
  P4IE |= BIT0;                             //ʹ��P4.0���ж�

  __bis_SR_register(LPM4_bits + GIE);       //����͹���ģʽ4 ���ж�
}

// P4�жϺ���
#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
{
	//P4OUT ^= BIT5;
	int i;
	for(i=10;i>0;i--){
		P4OUT ^= BIT5;
	    __delay_cycles(327600);
	}
	P4IFG &= ~BIT0;                          //��P4.0�жϱ�־λ
}