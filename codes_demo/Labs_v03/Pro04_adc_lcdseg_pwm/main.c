#include <msp430.h>
#include <stdint.h>
#include "dr_lcdseg.h"
#include "clock_setting.h"
#include "dr_dc_motor.h"

uint16_t adc0;

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
  UCSCTL2 = MCLK_FREQ / (XT2_FREQ / 16); //XT2频率较高，分频后作为基准可获得更高的精度
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
  //TB0设定，用于ADC触发脉冲
  TB0CTL = TBSSEL__SMCLK + MC__UP; // SMCLK, up-mode
  TB0CCR0 = SMCLK_FREQ / 6400 - 1; // 4MHz -> 6.4kHz
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
  adc0 = ADC12MEM0; //保存转换结果
  ADC12CTL0 &= ~ADC12ENC; //重置ADC，准备下一次采样
  ADC12CTL0 |= ADC12ENC;
}

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  _DINT();
  initClock();
  initADC();
  initLcdSeg();
  initDCMotor();
  _EINT();

  while(1)
  {
    int32_t val;
    val = adc0; //获取ADC转换结果
    val = val * PWM_COUNT * 2 / 4095 - PWM_COUNT; //将ADC转换结果从0~4095转换到-399~399
    LCDSEG_DisplayNumber(getRPM(), 0); //更新转速显示
    PWM_SetOutput(val); //修改PWM输出电压
    __delay_cycles(MCLK_FREQ / 10); //延时100ms
  }
}
