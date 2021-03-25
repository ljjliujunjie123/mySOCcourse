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

//初始化音频模块
void initAudio();

//关断音频输出
void audio_OutputShutDown();

//启动音频输出
void audio_OutputTurnOn();

//设定DA输出特定值
void audio_SetOutput(uint16_t val);

//复位解码器
void audio_DecoderReset();

//设定音量
void audio_SetVolume(uint8_t val); //1~16;

//获取当前解码器状态
const AudioDecoderStatus* audio_DecoderStatus();

//设定解码器的音频缓冲区，即数据来源
void audio_SetWavStream(Queue *stream);

#endif
