/*
 * main.c
 */
#include <msp430f6638.h>
void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 //关闭看门狗

  //使能XT2
  P7SEL |= BIT2+BIT3;                       //端口XT2的选择
  UCSCTL6 &= ~(XT2OFF);                     //打开XT2振荡器
  UCSCTL6 |= XCAP_3;

  do							//循环等待XT1和DOC振荡器稳定
  {
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
                                            // 清除XT1、XT2、DOC故障标志位
    SFRIFG1 &= ~OFIFG;                      // 清除OSC故障标志位
  }while (SFRIFG1&OFIFG);                   // 等待振荡器稳定

  UCSCTL6 &= ~(XT1DRIVE_3);                 // 减少驱动器

  //配置未使用的引脚
  P1OUT = 0x00;
  P2OUT = 0x00;
  P3OUT = 0x00;
  P4OUT = 0x00;
  P5OUT = 0x00;
  P6OUT = 0x00;
  P8OUT = 0x00;
  P1DIR = 0xFF;
  P2DIR = 0xFF;
  P3DIR = 0xFF;
  P4DIR = 0xFF;
  P5DIR = 0xFF;
  P6DIR = 0xFF;
  P8DIR = 0xFF;

  USBKEYPID = 0x9628;            //置KEYandPID为0x9628（允许访问USB配置寄存器）
  USBPWRCTL &= ~(SLDOEN+VUSBEN); //禁用VUSB、LDO和SLDO
  USBKEYPID = 0x9600;            //置KEYandPID为0x9600（禁止访问USB配置寄存器）

  //禁用SVS
  PMMCTL0_H = PMMPW_H;                //PMM密码
  SVSMHCTL &= ~(SVMHE+SVSHE);         //禁用高压测SVS
  SVSMLCTL &= ~(SVMLE+SVSLE);         //禁用低压测SVS

  __bis_SR_register(LPM1_bits);   	  //进入低功耗模式1
  __no_operation();                   //延时一个机器周期
}

