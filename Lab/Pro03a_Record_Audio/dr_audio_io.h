#ifndef __DR_AUDIO_IO_H_
#define __DR_AUDIO_IO_H_

#include "fw_queue.h"

#ifndef SMCLK_FREQ
  #define SMCLK_FREQ 20000000
#endif

#define INPUT_AUDIO_BUFFER_LENGTH 1024 //8K��������Լ125ms
#define OUTPUT_AUDIO_BUFFER_LENGTH 1024 //8K��������Լ62ms

extern Queue audio_input_stream; //��Ƶ���������ɴ��ж�ȡuint8_t����Ƶ�������
                                  //Ҳ��ʹ��audio_InputXXX����������ز���
extern Queue audio_output_stream; //��Ƶ������������в���uint16_t(��12λ��Ч)�Բ�����Ƶ���
                                   //Ҳ��ʹ��audio_OutputXXX����������ز���

void initAudioIO();

//�趨��Ƶģ��Ĳ����ʣ���λΪHz
void audio_SetSampleRate(uint32_t sr);

//���֮ǰ�Ĳ������
inline void audio_InputClear();

//��ȡ���size������������ʵ�ʲ�����
inline uint16_t audio_InputRead(uint8_t *buff, uint16_t size);

//����Ƶ�������һ��������
inline int audio_Output(uint16_t val);

//�����Ƶ�����������ʣ��ռ�(�Բ���������ʽ��ʾ)
inline uint16_t audio_OutputBufferSpace();

//---------------------------------------------------------------------
/* ��inline����ʵ�� */
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

static inline uint16_t audio_OutputBufferSpace() //��Ƶ��������������ɵĲ�����
{
  return (queue_CanWrite(&audio_output_stream) / 2);
}

#endif //__DR_AUDIO_IO_H_
