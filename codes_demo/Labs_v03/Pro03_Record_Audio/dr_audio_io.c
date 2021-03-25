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
  //�������趨
  initQueue(&audio_input_stream, audio_input_buffer, INPUT_AUDIO_BUFFER_LENGTH); //��ʼ����Ƶ����ʹ�õ���Ƶ������
  initQueue(&audio_output_stream, audio_output_buffer, OUTPUT_AUDIO_BUFFER_LENGTH); //��ʼ����Ƶ���ʹ�õ���Ƶ������
  //ADC�˷ſ���IO�趨
  AUDIO_INPUT_CTL_SET_DIR;
  AUDIO_INPUT_CTL_ENABLE; //ʹ��������˷�
  //TB0�趨������ADC�����������Ƶ���ʱ��
  TB0CTL = TBSSEL__SMCLK + MC__UP; // SMCLK, up-mode
  TB0CCR0 = SMCLK_FREQ / audio_sample_rate / 2 - 1; //���������õ�ģʽ��Ӧ���ٳ�2
  TB0CCTL0 = OUTMOD_4 + CCIE; //ʵ���������Ƶ��Ϊ����ƥ��Ƶ�ʵ�һ�룬�ж�Ƶ��Ϊ����ƥ��Ƶ��
  //ADC�趨
  P7SEL |= BIT5; //��֤û���������ģ���ź�����
  ADC12CTL1 = ADC12SHS_2 + ADC12SHP + ADC12CONSEQ_1; //TB0.0�������������ͨ������
  ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_13 + ADC12EOS; //3.3V��Դ�ο���A13��ͨ��ĩβ
  ADC12CTL0 = ADC12MSC + ADC12ON + ADC12SHT0_8 + ADC12ENC; //���ز�����ADC12ON��MEM0~MEM7ʹ��256���ڲ�����ENC
  ADC12IE = BIT0; //�����ж�(ͨ��0ת����ɺ��ж�)
  //DAC�˷ſ���IO�趨
  AUDIO_OUTPUT_CTL_ENABLE;;
  AUDIO_OUTPUT_CTL_SET_DIR;
  //����REF
  REFCTL0 |= REFON; //Ĭ�ϲ���1.5V
  //����DAC
  DAC12_0CTL0 &= ~DAC12ENC;
  DAC12_0CTL0 = DAC12OPS + DAC12SREF_0 + DAC12LSEL_0 + DAC12AMP_7 + DAC12IR; //P7.6, VREF+, 12bit, �����ģʽ, 1������
  DAC12_0CTL1 = DAC12OG; //��DAC12IRʱ2������
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
    led_SetLed(4, 1); //����LED4������¼�������г������������ʧʱ���Թ۲쵽
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
    led_SetLed(5, 0); //Ϩ��LED5
  }
  else
  {
    //AUDIO_OUTPUT_CTL_DISABLE;
    led_SetLed(5, 1); //����LED5���������Ź����г������������ж�����Թ۲쵽
  }
}
