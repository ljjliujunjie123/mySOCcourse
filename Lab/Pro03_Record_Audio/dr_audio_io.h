#ifndef __DR_AUDIO_IO_H_
#define __DR_AUDIO_IO_H_

#include "fw_queue.h"

#ifndef SMCLK_FREQ
  #define SMCLK_FREQ 20000000
#endif

#define INPUT_AUDIO_BUFFER_LENGTH 1024 //8K采样率下约125ms
#define OUTPUT_AUDIO_BUFFER_LENGTH 1024 //8K采样率下约62ms

extern Queue audio_input_stream; //音频输入流，可从中读取uint8_t的音频采样结果
                                  //也可使用audio_InputXXX函数进行相关操作
extern Queue audio_output_stream; //音频输出流，向其中插入uint16_t(低12位有效)以产生音频输出
                                   //也可使用audio_OutputXXX函数进行相关操作

void initAudioIO();

//设定音频模块的采样率，单位为Hz
void audio_SetSampleRate(uint32_t sr);

//清空之前的采样结果
inline void audio_InputClear();

//读取最多size个采样，返回实际采样数
inline uint16_t audio_InputRead(uint8_t *buff, uint16_t size);

//向音频输出插入一个采样点
inline int audio_Output(uint16_t val);

//检测音频输出缓冲区的剩余空间(以采样数的形式表示)
inline uint16_t audio_OutputBufferSpace();

//---------------------------------------------------------------------
/* 各inline函数实现 */
static inline void audio_InputClear()
{
  queue_ThrowData(&audio_input_stream, INPUT_AUDIO_BUFFER_LENGTH);
}

static inline uint16_t audio_InputRead(uint8_t *buff, uint16_t size)
{
  return queue_Read(&audio_input_stream, buff, size);
}

static inline int audio_Output(uint16_t val)
{
  return queue_Write(&audio_output_stream,
                      (uint8_t*)&val, sizeof(val));
}

static inline uint16_t audio_OutputBufferSpace() //音频输出缓冲区可容纳的采样数
{
  return (queue_CanWrite(&audio_output_stream) / 2);
}

#endif //__DR_AUDIO_IO_H_
