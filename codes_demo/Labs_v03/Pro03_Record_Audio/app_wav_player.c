#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include "app_wav_player.h"
#include "dr_sdcard.h"
#include "dr_tft.h"
#include "dr_audio_io.h"
#include "dr_buttons_and_leds.h"

//������������
#define BTNRC_NOTHING 0 //�������
#define BTNRC_NEXT    1 //Ҫ����һ�ļ�
#define BTNRC_EXIT    2 //Ҫ���˳�

//�����ļ����ŷ�����
#define PFRC_NOERR    0 //��������
#define PFRC_FMTERR   1 //��ʽ����
#define PFRC_EXIT     2 //Ҫ���˳�

FRESULT wp_rc;                                       /* Result code */
DIRS wp_dir;                                         /* Directory object */
FILINFO wp_fno;                                      /* File information object */
FIL wp_fil;                                          /* File object */

#define WP_BUF_SIZE 256
char wp_buf[WP_BUF_SIZE];

#define MAX_VOLUME 16
#define MAX_VOLUME_BIT 4
uint8_t curVolume = 1; //��ʼ����

uint8_t skipSample = 1; //�����ʹ���ʱ������������ʣ���ʾÿ�����������г�һ�����

static inline void strupr(char* str)
{
  char *ptr = str;
  while(!*ptr)
  {
    if(*ptr >= 'a' && *ptr <='z')
      *ptr -= 'a' - 'A';
    ptr++;
  }
}

void app_StartWavPlayer()
{
  etft_AreaSet(0,0,319,239,0);
  while(1)
  {
    wp_rc = f_opendir(&wp_dir, "");
    if(wp_rc)
    {
      etft_DisplayString("ERROR: No SDCard detected!", 0, 0, etft_Color(0xFF,0,0), 0);
    }

    etft_DisplayString("Audio Player", 112, 0, etft_Color(0,255,0), 0);
    for (;;)
    {
      wp_rc = f_readdir(&wp_dir, &wp_fno);                        // Read a directory item
      if (wp_rc || !wp_fno.fname[0]) break;                    // Error or end of dir
      if (wp_fno.fattrib & AM_DIR)                          //this is a directory
      {

      }
      else                                               //this is a file
      {
        strcpy(wp_buf, wp_fno.fname);
        strupr(wp_buf);

        if(strstr(wp_buf, ".WAV") == 0)
          continue; //����ļ���չ��

        if(app_PlayWavFile(wp_fno.fname) == PFRC_EXIT)
          return;
      }
    }
  }
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

  sprintf(wp_buf, "Channels = %d     ", audio_wav_fmt.wChannels);
  etft_DisplayString(wp_buf, 0, 48, 0xFFFF, 0); //��ʾ������
  sprintf(wp_buf, "SamplesPerSec = %5ld     ", audio_wav_fmt.dwSamplesPerSec);
  etft_DisplayString(wp_buf, 0, 64, 0xFFFF, 0); //��ʾ������
  sprintf(wp_buf, "BitsPerSample = %2d     ", audio_wav_fmt.wBitsPerSample);
  etft_DisplayString(wp_buf, 0, 80, 0xFFFF, 0); //��ʾÿ�������ı�����

  uint32_t outputSampleRate = audio_wav_fmt.dwSamplesPerSec;
  skipSample = 1;
  while(outputSampleRate > 16000)
  {
    outputSampleRate /= 2;
    skipSample *= 2;
  }

  audio_SetSampleRate(outputSampleRate);

  return 1;
}

void wp_DisplayPlayTime(uint16_t cur, uint16_t all)
{
  sprintf(wp_buf, "%02d:%02d/%02d:%02d", cur / 60
          , cur % 60, all / 60, all % 60);
  etft_DisplayString(wp_buf, 0, 32, 0xFFFF, 0); //��ʾ��ǰ����
}

void wp_DisplayVolume()
{
  sprintf(wp_buf, "Volume = %02d    ", curVolume);
  etft_DisplayString(wp_buf, 0, 96, 0xFFFF, 0); //��ʾ��ǰ����
}

int wp_ProcessBtns()
{
  if(btn_Pressed(3))
  {
    curVolume++;
    if(curVolume > MAX_VOLUME)
      curVolume = MAX_VOLUME;
    wp_DisplayVolume();
    return BTNRC_NOTHING;
  }

  if(btn_Pressed(7))
  {
    if(curVolume > 0)
      curVolume--;;
    wp_DisplayVolume();
    return BTNRC_NOTHING;
  }

  if(btn_Pressed(5))
  {
    return BTNRC_NEXT;
  }

  if(btn_Pressed(4))
  {
    return BTNRC_EXIT;
  }

  return BTNRC_NOTHING;
}

int app_PlayWavFile(const char* filename)
{
  etft_AreaSet(0,16,319,239,0);
  wp_rc = f_open(&wp_fil, filename, FA_READ); //���ļ�

  wp_DisplayVolume();

  sprintf(wp_buf, "%-30s", wp_fno.fname);
  etft_DisplayString(wp_buf, 0, 16, 0xFFFF, 0);
  etft_DisplayString("S7 : Vol-      S3 : Vol+      ", 320 - 240, 240 - 32, 0xFFFF, 0);
  etft_DisplayString("S5 : Next      S4 : Exit      ", 320 - 240, 240 - 16, 0xFFFF, 0);

  //��ȡRIFF�ļ�ͷ
  UINT size;
  wp_rc = f_read(&wp_fil, &audio_riff_header, sizeof(audio_riff_header), &size); /* Read a chunk of file */
  if(wp_rc || (size != sizeof(audio_riff_header)))
    return PFRC_FMTERR; /* Error or end of file */
  if(!audio_CheckRIFFHeader())
    return PFRC_FMTERR; //�ļ�ͷУ��ʧ��

  int fmtReaded = 0;
  while(1) //��ȡ����CHUNK
  {
    wp_rc = f_read(&wp_fil, &curChunkHeader, sizeof(curChunkHeader), &size); /* Read a chunk of file */
    if(wp_rc || (size != sizeof(curChunkHeader)))
      return PFRC_FMTERR; /* Error or end of file */

    if(memcmp(curChunkHeader.szID, "fmt ", 4) == 0) //��ʽCHUNK
    {
      fmtReaded = 1;
      wp_rc = f_read(&wp_fil, &audio_wav_fmt, curChunkHeader.dwChunkSize, &size); /* Read a chunk of file */
      if(wp_rc || (size != curChunkHeader.dwChunkSize))
        return PFRC_FMTERR; /* Error or end of file */
      if(!audio_CheckWavFmt())
        return PFRC_FMTERR; //��ʽ��֧��
    }
    else if(memcmp(curChunkHeader.szID, "data", 4) == 0) //�Ѿ���data��
    {
      break; //��ͨCHUNK��ȡ����������while��ʼ����data
    }
    else //����CHUNK������
    {
      wp_rc = f_lseek(&wp_fil, wp_fil.fptr + curChunkHeader.dwChunkSize);
      if(wp_rc) return PFRC_FMTERR; //�ļ���С������
    }
  }

  if(!fmtReaded)
    return PFRC_FMTERR; //ȱ��fmt��

  uint32_t curDataPtr = 0; //��ǰ������data�εĶ೤����
  uint32_t sampleCnt = 0; //��һ�����������ж��ٸ�����
  uint16_t curSec = 0; //��ǰ���ŵ�������
  uint16_t fileSec = curChunkHeader.dwChunkSize / audio_wav_fmt.dwAvgBytesPerSec; //ȫ�ļ�����
  uint16_t decodedSampleCount = WP_BUF_SIZE / audio_wav_fmt.wBlockAlign / skipSample; //ÿ�ν������Ĳ�����
  int skipCnter = 1;

  wp_DisplayPlayTime(curSec, fileSec); //��ʼ��ʾ
  while(curDataPtr < curChunkHeader.dwChunkSize)
  {
    while(decodedSampleCount > audio_OutputBufferSpace()) //��ʱ�ȴ��������пռ��ټ���
    {
      int rc = wp_ProcessBtns();
      if(rc == BTNRC_NEXT)
      {
        f_close(&wp_fil); //�ر��ļ�
        return PFRC_NOERR;
      }
      if(rc == BTNRC_EXIT)
      {
        f_close(&wp_fil); //�ر��ļ�
        return PFRC_EXIT;
      }
    }

    wp_rc = f_read(&wp_fil, &wp_buf, WP_BUF_SIZE, &size); /* Read a chunk of file */
    if(wp_rc || (size <= 0))
      break; //�������

    curDataPtr += size;
    uint8_t* temp = (uint8_t*)wp_buf;
    uint16_t vol[2];
    while(size > 0)
    {
      if(skipCnter >= skipSample)
      {
        skipCnter = 1;
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
          vol[0] = (vol[0] + vol[1]) >> 1; //��Ϊ˫��������ȡƽ��

        vol[0] = (vol[0] * curVolume) >> MAX_VOLUME_BIT; //��������
        audio_Output(vol[0]); //�趨DA���
      }
      else
        skipCnter++;

      size -= audio_wav_fmt.wBlockAlign;
      temp += audio_wav_fmt.wBlockAlign;
      sampleCnt++; //����������
    }

    if(sampleCnt > audio_wav_fmt.dwSamplesPerSec) //��1s
    {
      curSec++;
      sampleCnt -= audio_wav_fmt.dwSamplesPerSec;
      wp_DisplayPlayTime(curSec, fileSec);
    }
  }

  f_close(&wp_fil); //�ر��ļ�
  return PFRC_NOERR;
}
