/*
 * main.c
 */
#include <msp430f6638.h>
void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;    // �رտ��Ź�
  P4DIR |= BIT5;                // ����4.5��Ϊ���ģʽ
  P4OUT |= BIT0;				// ѡ��P4.0Ϊ�����ʽ
  P4REN |= BIT0;				// P4.0ʹ��

  while (1)                   // Test P1.4
  {
    if (P4IN & BIT0)		//���P4.0Ϊ1��ִ�У����ǲ�ѯ��ʽ����ȥ���ǵͣ�����Ϊ��
    	P4OUT |= BIT5;     //ʹP4.5�ø�
    else
     P4OUT &= ~BIT5;     // else reset
  }
}
