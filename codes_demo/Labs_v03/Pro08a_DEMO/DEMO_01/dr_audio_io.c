#include <msp430.h>
#include "dr_audio_io.h"
#include "key_led/dr_buttons_and_leds.h"

#define AUDIO_OUTPUT_CTL_SET_DIR    P5DIR |= BIT6
#define AUDIO_OUTPUT_CTL_ENABLE     P5OUT &=~BIT6
#define AUDIO_OUTPUT_CTL_DISABLE    P5OUT |= BIT6

uint8_t audio_output_buffer[OUTPUT_AUDIO_BUFFER_LENGTH];
Queue audio_output_stream;

uint32_t audio_sample_rate = 8000;

void initAudioIO()
{
  //缓冲区设定
  initQueue(&audio_output_stream, audio_output_buffer, OUTPUT_AUDIO_BUFFER_LENGTH); //初始化音频输出使用的音频缓冲区
  //TB0设定，用于ADC触发脉冲和音频输出时钟
  TB0CTL = TBSSEL__SMCLK + MC__UP; // SMCLK, up-mode
  TB0CCR0 = SMCLK_FREQ / audio_sample_rate / 2 - 1; //由于所采用的模式，应当再除2
  TB0CCTL0 = OUTMOD_4 + CCIE; //实际脉冲输出频率为周期匹配频率的一半，中断频率为周期匹配频率
  //DAC运放控制IO设定
  AUDIO_OUTPUT_CTL_ENABLE;;
  AUDIO_OUTPUT_CTL_SET_DIR;
  //配置REF
  REFCTL0 |= REFON; //默认参数1.5V
  //配置DAC
  DAC12_0CTL0 &= ~DAC12ENC;
  DAC12_0CTL0 = DAC12OPS + DAC12SREF_0 + DAC12LSEL_0 + DAC12AMP_7 + DAC12IR; //P7.6, VREF+, 12bit, 最高速模式, 1倍增益
  DAC12_0CTL1 = DAC12OG; //无DAC12IR时2倍增益
  DAC12_0CTL0 |= DAC12ENC;
}

void audio_SetSampleRate(uint32_t sr)
{
  audio_sample_rate = sr;
  TB0CCR0 = SMCLK_FREQ / audio_sample_rate / 2 - 1;
}

#pragma vector=TIMER0_B0_VECTOR
__interrupt void audio_output_isr (void)
{
  static int cnter = 0;
  cnter = 1 - cnter;
  if(cnter)
    return;

  static uint16_t val;
  if(queue_CanRead(&audio_output_stream) >= sizeof(val))
  {
    queue_Read(&audio_output_stream,
               (uint8_t*)&val, sizeof(val));
    DAC12_0DAT = val;
    //AUDIO_OUTPUT_CTL_ENABLE;
    //led_SetLed(5, 0); //熄灭LED5
  }
  else
  {
    //AUDIO_OUTPUT_CTL_DISABLE;
    //led_SetLed(5, 1); //点亮LED5，这样播放过程中出现输入数据中断则可以观察到
  }
}
