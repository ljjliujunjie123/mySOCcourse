#include <msp430.h>
#include <stdint.h>
#include "dr_step_motor.h"
#include "dr_lcdseg.h"

int adc0;

void initClock()
{
  while(BAKCTL & LOCKIO) // Unlock XT1 pins for operation
    BAKCTL &= ~(LOCKIO);
  UCSCTL6 &= ~XT1OFF; //启动XT1
  P7SEL |= BIT2 + BIT3; //XT2引脚功能选择
  UCSCTL6 &= ~XT2OFF; //启动XT2
  while (SFRIFG1 & OFIFG) { //等待XT1、XT2与DCO稳定
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }

  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__XT2CLK; //避免DCO调整中跑飞

  UCSCTL1 = DCORSEL_5; //6000kHz~23.7MHz
  UCSCTL2 = 16000000 / (4000000 / 16); //XT2频率较高，分频后作为基准可获得更高的精度
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2进行16分频后作为基准
  while (SFRIFG1 & OFIFG) { //等待XT1、XT2与DCO稳定
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //设定几个CLK的分频
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__DCOCLK; //设定几个CLK的时钟源
}

void initADC()
{
  //内存变量初始化
  adc0 = 0;
  //IO设定
  P7SEL |= BIT7; //保证没有输出干扰模拟信号输入
  //TB0设定
  TB0CTL = TBSSEL__SMCLK + MC__UP; // SMCLK, up-mode
  TB0CCR0 = 4000000 / 6400 - 1; // 4MHz -> 6.4kHz
  TB0CCTL0 = OUTMOD_4; //实际只有3.2kHz
  //ADC设定
  ADC12CTL1 = ADC12SHS_2 + ADC12SHP + ADC12CONSEQ_1; //TB0.0，脉冲采样，多通道单次
  ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_15 + ADC12EOS; //3.3V电源参考，A15，通道末尾
  ADC12CTL0 = ADC12MSC + ADC12ON + ADC12SHT0_8 + ADC12ENC; //多重采样，ADC12ON，MEM0~MEM7使用256周期采样，ENC
  ADC12IE = BIT0; //开启中断(通道0转换完成后中断)
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  adc0 = ADC12MEM0;
  ADC12CTL0 &= ~ADC12ENC;
  ADC12CTL0 |= ADC12ENC;
}

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  _DINT();
  initClock();
  initADC();
  initStepMotor();
  initLcdSeg();
  _EINT();

  StepMotor_SetEnable(1);

  //按键IO初始化
  P4REN |= 0x1F; //使能按键端口上的上下拉电阻
  P4OUT |= 0x1F; //上拉状态

  uint8_t last_btn = 0x1F, cur_btn, temp;
  while(1)
  {
    cur_btn = P4IN & 0x1F;
    temp = (cur_btn ^ last_btn) & last_btn; //找出刚向下跳变的按键
    last_btn = cur_btn;

    int16_t rpm = ((int32_t)adc0) * 600L / 4096L;
    LCDSEG_DisplayNumber(rpm, 0);
    StepMotor_SetRPM(rpm);

    if(temp & BIT3) //S4
      StepMotor_AddStep(360000);
    else if(temp & BIT1) //S6
      StepMotor_AddStep(-360000);
    else if(temp & BIT2) //S5
      StepMotor_ClearStep();
    else if(temp & BIT0) //S7
      StepMotor_AddStep(-1);
    else if(temp & BIT4) //S3
      StepMotor_AddStep(1);

    __delay_cycles(1600000);
  }
}
