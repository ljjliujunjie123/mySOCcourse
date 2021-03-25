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
  WDTCTL = WDTPW + WDTHOLD;     // 关闭看门狗
  P4DIR |= BIT5;                // P4.5/LED 设为输出
  CBCTL0 |= CBIPEN + CBIPSEL_0; // 使能正端, 输入通道选择 CB0(连接P6.0)
  CBCTL1 |= CBPWRMD_1;          // 正常的电源供应模式
  CBCTL2 |= CBRSEL;             // 使能内部参考电压选择
  CBCTL2 |= CBRS_3+CBREFL_1;    // 参考电压选择为1.5V
  CBCTL3 |= BIT0;               // 关闭CB0输入缓存
  __delay_cycles(75);
  CBINT &= ~(CBIFG + CBIIFG);   // 清除中断标志位
  CBINT  |= CBIE;               // 使能比较中断，边缘选择为上升沿
  CBCTL1 |= CBON;               // 打开比较器
  __bis_SR_register(LPM4_bits+GIE); // 打开总中断，进入LPM4模式
  __no_operation();
}


#pragma vector=COMP_B_VECTOR
__interrupt void Comp_B_ISR (void)
{
  CBCTL1 ^= CBIES;              // 翻转触发边缘
  CBINT &= ~CBIFG;              // 清除中断标志
  P4OUT ^= BIT5;                // 翻转LED5状态
}
