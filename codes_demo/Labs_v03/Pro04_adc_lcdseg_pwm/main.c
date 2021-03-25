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
  UCSCTL6 &= ~XT1OFF; //����XT1
  P7SEL |= BIT2 + BIT3; //XT2���Ź���ѡ��
  UCSCTL6 &= ~XT2OFF; //����XT2
  while (SFRIFG1 & OFIFG) { //�ȴ�XT1��XT2��DCO�ȶ�
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }

  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__XT2CLK; //����DCO�������ܷ�

  UCSCTL1 = DCORSEL_5; //6000kHz~23.7MHz
  UCSCTL2 = MCLK_FREQ / (XT2_FREQ / 16); //XT2Ƶ�ʽϸߣ���Ƶ����Ϊ��׼�ɻ�ø��ߵľ���
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2����16��Ƶ����Ϊ��׼
  while (SFRIFG1 & OFIFG) { //�ȴ�XT1��XT2��DCO�ȶ�
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //�趨����CLK�ķ�Ƶ
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__DCOCLK; //�趨����CLK��ʱ��Դ
}

void initADC()
{
  //�ڴ������ʼ��
  adc0 = 0;
  //IO�趨
  P7SEL |= BIT7; //��֤û���������ģ���ź�����
  //TB0�趨������ADC��������
  TB0CTL = TBSSEL__SMCLK + MC__UP; // SMCLK, up-mode
  TB0CCR0 = SMCLK_FREQ / 6400 - 1; // 4MHz -> 6.4kHz
  TB0CCTL0 = OUTMOD_4; //ʵ��ֻ��3.2kHz
  //ADC�趨
  ADC12CTL1 = ADC12SHS_2 + ADC12SHP + ADC12CONSEQ_1; //TB0.0�������������ͨ������
  ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_15 + ADC12EOS; //3.3V��Դ�ο���A15��ͨ��ĩβ
  ADC12CTL0 = ADC12MSC + ADC12ON + ADC12SHT0_8 + ADC12ENC; //���ز�����ADC12ON��MEM0~MEM7ʹ��256���ڲ�����ENC
  ADC12IE = BIT0; //�����ж�(ͨ��0ת����ɺ��ж�)
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  adc0 = ADC12MEM0; //����ת�����
  ADC12CTL0 &= ~ADC12ENC; //����ADC��׼����һ�β���
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
    val = adc0; //��ȡADCת�����
    val = val * PWM_COUNT * 2 / 4095 - PWM_COUNT; //��ADCת�������0~4095ת����-399~399
    LCDSEG_DisplayNumber(getRPM(), 0); //����ת����ʾ
    PWM_SetOutput(val); //�޸�PWM�����ѹ
    __delay_cycles(MCLK_FREQ / 10); //��ʱ100ms
  }
}
