#ifndef __DR_AUDIO_H_
#define __DR_AUDIO_H_

#include "fw_queue.h"

#ifndef SMCLK_FREQ
#define SMCLK_FREQ 20000000 
#endif

#define ADI_INVALID_FILE 0x01
#define ADI_PLAYING      0x02

typedef struct
{
  uint16_t wFlags;
  uint16_t wChannels;
  uint32_t dwSamplesPerSec;
  uint16_t wBitsPerSample;
  uint16_t wCurPosSec;
  uint16_t wFullSec;
  uint32_t dwLostCount;
} AudioDecoderStatus;

//��ʼ����Ƶģ��
void initAudio();

//�ض���Ƶ���
void audio_OutputShutDown();

//������Ƶ���
void audio_OutputTurnOn();

//�趨DA����ض�ֵ
void audio_SetOutput(uint16_t val);

//��λ������
void audio_DecoderReset();

//�趨����
void audio_SetVolume(uint8_t val); //1~16;

//��ȡ��ǰ������״̬
const AudioDecoderStatus* audio_DecoderStatus();

//�趨����������Ƶ����������������Դ
void audio_SetWavStream(Queue *stream);

#endif
