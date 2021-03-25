/*
 * main.c
 */
#include <msp430f6638.h>

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

  __no_operation();                         //�ղ���
}

// P4�жϺ���
#pragma vector=PORT4_VECTOR
__interrupt void Port_4(void)
{
  __delay_cycles(10000);
  if ((P4IN & BIT0)==0){
      P4OUT ^= BIT5;                       //�ı�LED5��״̬
  }
  P4IFG &= ~BIT0;                          //��P4.0�жϱ�־λ
}
