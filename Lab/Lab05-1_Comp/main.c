//******************************************************************************
// MSP430f663x Demo - COMPB interrupt capability; Vcompare is compared against
//                    internal 1.5V reference
//
// Description: Use CompB and internal reference to determine if input'Vcompare'
//    is high of low.  For the first time, when Vcompare exceeds the 1.5V internal
//	  reference, CBIFG is set and device enters the CompB ISR. In the ISR CBIES is
//	  toggled such that when Vcompare is less than 1.5V internal reference, CBIFG is set.
//    LED is toggled inside the CompB ISR
//
//                 MSP430f663x
//             ------------------
//         /|\|                  |
//          | |                  |
//          --|RST      P6.0/CB0 |<--Vcompare
//            |                  |
//            |            P4.5  |--> LED 'ON'(Vcompare>1.5V); 'OFF'(Vcompare<1.5V)
//            |                  |
//
//   Texas Instruments Inc.
//   April 2012
//******************************************************************************
#include <msp430f6638.h>

void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;     // �رտ��Ź�
  P4DIR |= BIT5;                // P4.5/LED ��Ϊ���
  CBCTL0 |= CBIPEN + CBIPSEL_0; // ʹ������, ����ͨ��ѡ�� CB0(����P6.0)
  CBCTL1 |= CBPWRMD_1;          // �����ĵ�Դ��Ӧģʽ
  CBCTL2 |= CBRSEL;             // ʹ���ڲ��ο���ѹѡ��
  CBCTL2 |= CBRS_3+CBREFL_1;    // �ο���ѹѡ��Ϊ1.5V
  CBCTL3 |= BIT0;               // �ر�CB0���뻺��
  __delay_cycles(75);
  CBINT &= ~(CBIFG + CBIIFG);   // ����жϱ�־λ
  CBINT  |= CBIE;               // ʹ�ܱȽ��жϣ���Եѡ��Ϊ������
  CBCTL1 |= CBON;               // �򿪱Ƚ���
  __bis_SR_register(LPM4_bits+GIE); // �����жϣ�����LPM4ģʽ
  __no_operation();
}


#pragma vector=COMP_B_VECTOR
__interrupt void Comp_B_ISR (void)
{
  CBCTL1 ^= CBIES;              // ��ת������Ե
  CBINT &= ~CBIFG;              // ����жϱ�־
  P4OUT ^= BIT5;                // ��תLED5״̬
}
