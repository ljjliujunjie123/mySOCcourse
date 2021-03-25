#include <msp430.h>
#include <stdint.h>
#include <string.h>
#include "dr_audio.h"

#define WAIT_STREAM 65535
#define THROW_DATA 65534
#define READ_RIFF_HEADER 0
#define READ_CHUNK_HEADER 1
#define READ_CHUNK 2
#define READ_DATA 3

#define MAX_CHUNK_LENGTH 18

struct WAV_STATUS
{
  uint16_t status; //��ǰ״̬��־
  uint32_t fpos; //��ǰ�ļ�ָ��λ��
  Queue* stream; //��Ƶ������
  uint32_t sample_count; //��Ƶ�����ܵĲ�����
  uint32_t cur_sample; //��ǰ���ŵ��Ĳ�����
  uint32_t lost_count; //�ж��ٴβ�����ʧ(�����н��붨ʱ�жϵ������ݿɶ�)
} audio_wav_status;

uint32_t curSampFreq = 8000; //Ĭ�ϲ����ʣ��ᰴ�����ļ��ı�
#define MAX_VOLUME 16
#define MAX_VOLUME_BIT 4
uint8_t curVolume = 1; //��ʼ����

void initAudio()
{
  //���ÿ��ƶ˿�
  P5OUT |= BIT6;
  P5DIR |= BIT6;
  
  //����REF
  REFCTL0 |= REFON; //Ĭ�ϲ���1.5V 
  
  //����DAC
  DAC12_0CTL0 &= ~DAC12ENC;
  DAC12_0CTL0 = DAC12OPS + DAC12SREF_0 + DAC12LSEL_0 + DAC12AMP_7 + DAC12IR; //P7.6, VREF+, 12bit, �����ģʽ, 1������
  DAC12_0CTL1 = DAC12OG; //��DAC12IRʱ2������
  DAC12_0CTL0 |= DAC12ENC;
  audio_SetOutput(0);
  
  //����ʱ��
  TA2CTL = TASSEL__SMCLK + MC__UP + ID__1;
  TA2CCR0 = (SMCLK_FREQ / curSampFreq) - 1;
  TA2CCTL0 = CCIE;
  
  //����wav������״̬
  audio_wav_status.status = WAIT_STREAM;
  audio_wav_status.lost_count = 0;
}

void audio_SetWavStream(Queue *stream)
{
  audio_wav_status.fpos = 0;
  audio_wav_status.stream = stream;
  audio_wav_status.status = READ_RIFF_HEADER;
}

void audio_OutputShutDown()
{
  P5OUT |= BIT6;
}

void audio_OutputTurnOn()
{
  P5OUT &= ~BIT6;
}

void audio_SetOutput(uint16_t val)
{
  DAC12_0DAT = val;
}

struct RIFF_HEADER
{
  char szRiffID[4]; // 'R','I','F','F'
  uint32_t dwRiffSize;
  char szRiffFormat[4]; // 'W','A','V','E'
} audio_riff_header;

typedef struct
{
  char szID[4];
  uint32_t dwChunkSize;
} RIFF_CHUNK_HEADER;

struct WAV_FMT_CHUNK
{
  //RIFF_CHUNK_HEADER chHeader; // 'f','m','t',' '
  uint16_t wFormatTag;
  uint16_t wChannels;
  uint32_t dwSamplesPerSec;
  uint32_t dwAvgBytesPerSec;
  uint16_t wBlockAlign;
  uint16_t wBitsPerSample; 
  uint16_t wExtInfo;
} audio_wav_fmt;

RIFF_CHUNK_HEADER curChunkHeader;

const AudioDecoderStatus* audio_DecoderStatus()
{
  static AudioDecoderStatus adi;
  
  adi.wFlags = 0;
  if(audio_wav_status.status == THROW_DATA)
    adi.wFlags |= ADI_INVALID_FILE;
  else if(audio_wav_status.status == READ_DATA)
    adi.wFlags |= ADI_PLAYING;
  
  adi.wChannels = audio_wav_fmt.wChannels;
  adi.dwSamplesPerSec = audio_wav_fmt.dwSamplesPerSec;
  adi.wBitsPerSample = audio_wav_fmt.wBitsPerSample;
  
  adi.wCurPosSec = audio_wav_status.cur_sample / audio_wav_fmt.dwSamplesPerSec;
  adi.wFullSec = audio_wav_status.sample_count / audio_wav_fmt.dwSamplesPerSec;
  adi.dwLostCount = audio_wav_status.lost_count;
  
  return &adi;
}

void audio_DecoderReset()
{
  audio_wav_status.status = READ_RIFF_HEADER;
  audio_wav_status.fpos = 0;
  audio_OutputShutDown();
  audio_wav_fmt.wFormatTag = 0; //�����ʽ��־����֤�´β���ǰȷʵ������fmt
  uint16_t size = queue_CanRead(audio_wav_status.stream);
  queue_ThrowData(audio_wav_status.stream, size);
}

int audio_CheckRIFFHeader()
{
  if(memcmp(audio_riff_header.szRiffID, "RIFF", 4) != 0)
    return 0;
  if(memcmp(audio_riff_header.szRiffFormat, "WAVE", 4) != 0)
    return 0;
  return 1;
}

int audio_CheckWavFmt()
{
  if(audio_wav_fmt.wFormatTag != 1)
    return 0;
  if(audio_wav_fmt.wChannels != 1 && audio_wav_fmt.wChannels != 2)
    return 0;
  if(audio_wav_fmt.wBlockAlign > 4 && audio_wav_fmt.wBlockAlign == 0)
    return 0;
  if(audio_wav_fmt.wBitsPerSample != 8 && audio_wav_fmt.wBitsPerSample != 16)
    return 0;
  
  curSampFreq = audio_wav_fmt.dwSamplesPerSec;
  TA2CCR0 = (SMCLK_FREQ / curSampFreq) - 1;
  return 1;
}

void audio_ReadHeader(uint16_t size)
{
  switch(audio_wav_status.status)
  {
  case READ_RIFF_HEADER:
    if(size >= sizeof(audio_riff_header))
    {
      queue_Read(audio_wav_status.stream, (uint8_t*)&audio_riff_header, sizeof(audio_riff_header));
      if(audio_CheckRIFFHeader())
      {
        audio_wav_status.status = READ_CHUNK_HEADER;
        audio_wav_status.fpos = 4;
      }
      else
      {
        audio_wav_status.status = THROW_DATA; //�ļ���֧�֣���������
      }
    }
    return;
    
  case READ_CHUNK_HEADER: //��ȡһ��CHUNK��ͷ����ȡCHUNK���ȣ���������data�������ʽ����벥��״̬
    if(size >= sizeof(curChunkHeader))
    {
      audio_wav_status.fpos += queue_Read(audio_wav_status.stream, (uint8_t*)&curChunkHeader, sizeof(curChunkHeader));
      if(memcmp(curChunkHeader.szID, "data", 4) != 0)
        audio_wav_status.status = READ_CHUNK;
      else
      {
        if(audio_CheckWavFmt())
        {
          audio_wav_status.sample_count = curChunkHeader.dwChunkSize / audio_wav_fmt.wBlockAlign;
          audio_wav_status.cur_sample = 0;
          audio_wav_status.status = READ_DATA;
          audio_OutputTurnOn();
        }
        else
        {
          audio_wav_status.status = THROW_DATA; //�ļ���֧�֣���������
        }
      }
    }
    return;
    
  case READ_CHUNK: //��ȡһ��CHUNK������fmt�򱣴�����������ֱ��������CHUNK
    if(curChunkHeader.dwChunkSize > MAX_CHUNK_LENGTH)
    {
      audio_wav_status.status = THROW_DATA; //�ļ���֧�֣���������
      return;
    }
    
    if(size >= curChunkHeader.dwChunkSize)
    {
      if(memcmp(curChunkHeader.szID, "fmt ", 4) == 0)
      {
        audio_wav_status.fpos += queue_Read(audio_wav_status.stream, (uint8_t*)&audio_wav_fmt, curChunkHeader.dwChunkSize);
      }
      else
      {
        audio_wav_status.fpos += queue_ThrowData(audio_wav_status.stream, curChunkHeader.dwChunkSize);
      }
      audio_wav_status.status = READ_CHUNK_HEADER;
    }
    return;
    
  default:
    audio_DecoderReset();
    return;
  }
}

void audio_Decode() //������Ƶ����
{
  uint8_t temp[4];
  uint16_t vol[2];
  audio_wav_status.fpos += queue_Read(audio_wav_status.stream, temp, audio_wav_fmt.wBlockAlign);

  //����ʵ��������Ŀ�������������������ȣ�����Ҫ�Ĳ��ֲ���Ӱ�����
  if(audio_wav_fmt.wBitsPerSample == 8)
  { //ÿ����8λ
    vol[0] = (temp[0] << 4) & 0x0FF0;
    vol[1] = (temp[1] << 4) & 0x0FF0;
  }
  else //ÿ����16λ
  {
    vol[0] = temp[0] | (temp[1] << 8);
    vol[0] += 0x8000; //16λ��Ϊ�з���������Ҫ����ƫ�ñ�Ϊ�޷�����
    vol[0] = (vol[0] >> 4) & 0x0FFF;
    vol[1] = temp[2] | (temp[3] << 8);
    vol[1] += 0x8000; //����ƫ��
    vol[1] = (vol[1] >> 4) & 0x0FFF;
  }
  
  if(audio_wav_fmt.wChannels == 2)
  {
    vol[0] = (vol[0] + vol[1]) >> 1; //��Ϊ˫��������ȡƽ��
  }
  
  vol[0] = (vol[0] * curVolume) >> MAX_VOLUME_BIT; //��������
  audio_SetOutput(vol[0]); //�趨DA���
  audio_wav_status.cur_sample++; //��¼�Ѿ����ŵĲ�����
}

void audio_SetVolume(uint8_t val)
{
  if(val > MAX_VOLUME)
    val = MAX_VOLUME;
  curVolume = val;
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void audioTimerInterrupt()
{
  if(audio_wav_status.status == WAIT_STREAM)
    return;
  
  static uint32_t idleCounter = 0;
  uint16_t size = queue_CanRead(audio_wav_status.stream);
  
  if(audio_wav_status.status == READ_DATA) //����״̬��������д����ǰ�����ڴ����ٶ�
  {
    if(size >= audio_wav_fmt.wBlockAlign) //��Ƶ���������г���1������
    {
      audio_Decode();
      idleCounter = 0;
      if(audio_wav_status.fpos >= audio_riff_header.dwRiffSize)
      { //������ɸ�λ
        audio_DecoderReset();
      }
    }
    else
    {
      audio_wav_status.lost_count++; //����״̬������ʧ
    }
    return;
  }
  
  if(size == 0) //����������
  {
    if(idleCounter < curSampFreq / 2) //ͨѶ��λʱ��0.5s
    {
      idleCounter++;
    }
    else
    { //ʧȥͨѶ��λ
      audio_DecoderReset();
    }
    return;
  }

  idleCounter = 0;
  if(audio_wav_status.status == THROW_DATA)
  { //��������״̬��ֱ�Ӱ�����������ʲôλ�ú�Ͳ�����
    queue_ThrowData(audio_wav_status.stream, size);
    return;
  }
  
  audio_ReadHeader(size); //����ǰ������һ��״̬�����Ƕ�ȡ�ļ�ͷ״̬
}
