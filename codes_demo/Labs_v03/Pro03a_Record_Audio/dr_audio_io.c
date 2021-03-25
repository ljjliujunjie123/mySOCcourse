#include <msp430.h>
#include "dr_audio_io.h"
#include "dr_buttons_and_leds.h"

#define AUDIO_OUTPUT_CTL_SET_DIR    P5DIR |= BIT6
#define AUDIO_OUTPUT_CTL_ENABLE     P5OUT &=~BIT6
#define AUDIO_OUTPUT_CTL_DISABLE    P5OUT |= BIT6

#define AUDIO_INPUT_CTL_SET_DIR     P5DIR |= BIT1
#define AUDIO_INPUT_CTL_ENABLE      P5OUT |= BIT1
#define AUDIO_INPUT_CTL_DISABLE     P5OUT &=~BIT1

uint8_t audio_output_buffer[OUTPUT_AUDIO_BUFFER_LENGTH];
Queue audio_output_stream;
uint8_t audio_input_buffer[INPUT_AUDIO_BUFFER_LENGTH];
Queue audio_input_stream;

uint32_t audio_sample_rate = 8000;

void initAudioIO()
{
  //缓冲区设定
  initQueue(&audio_input_stream, audio_input_buffer, INPUT_AUDIO_BUFFER_LENGTH); //初始化音频输入使用的音频缓冲区
  initQueue(&audio_output_stream, audio_output_buffer, OUTPUT_AUDIO_BUFFER_LENGTH); //初始化音频输出使用的音频缓冲区
  //ADC运放控制IO设定
  AUDIO_INPUT_CTL_SET_DIR;
  AUDIO_INPUT_CTL_ENABLE; //使能输入侧运放
  //TB0设定，用于ADC触发脉冲和音频输出时钟
  TB0CTL = TBSSEL__SMCLK + MC__UP; // SMCLK, up-mode
  TB0CCR0 = SMCLK_FREQ / audio_sample_rate / 2 - 1; //由于所采用的模式，应当再除2
  TB0CCTL0 = OUTMOD_4 + CCIE; //实际脉冲输出频率为周期匹配频率的一半，中断频率为周期匹配频率
  //ADC设定
  P7SEL |= BIT5; //保证没有输出干扰模拟信号输入
  ADC12CTL1 = ADC12SHS_2 + ADC12SHP + ADC12CONSEQ_1; //TB0.0，脉冲采样，多通道单次
  ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_13 + ADC12EOS; //3.3V电源参考，A13，通道末尾
  ADC12CTL0 = ADC12MSC + ADC12ON + ADC12SHT0_8 + ADC12ENC; //多重采样，ADC12ON，MEM0~MEM7使用256周期采样，ENC
  ADC12IE = BIT0; //开启中断(通道0转换完成后中断)
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

#pragma vector=ADC12_VECTOR
__interrupt void audio_input_isr (void)
{
  static uint8_t temp;
  temp = ((ADC12MEM0) >> 4) & 0xFF;

  if(queue_CanWrite(&audio_input_stream) < sizeof(temp))
  {
    led_SetLed(4, 1); //点亮LED4，这样录音过程中出现输入采样丢失时可以观察到
    queue_ThrowData(&audio_input_stream, sizeof(temp));
  }
  else
  {
    led_SetLed(4, 0);
  }

  queue_Write(&audio_input_stream, &temp, sizeof(temp));

  ADC12CTL0 &= ~ADC12ENC;
  ADC12CTL0 |= ADC12ENC;
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
    led_SetLed(5, 0); //熄灭LED5
  }
  else
  {
    //AUDIO_OUTPUT_CTL_DISABLE;
    led_SetLed(5, 1); //点亮LED5，这样播放过程中出现输入数据中断则可以观察到
  }
}
