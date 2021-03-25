#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include "app_wav_player.h"
#include "dr_sdcard.h"
#include "dr_tft.h"
#include "dr_audio_io.h"
#include "dr_buttons_and_leds.h"

//按键处理返回码
#define BTNRC_NOTHING 0 //无需操作
#define BTNRC_NEXT    1 //要求下一文件
#define BTNRC_EXIT    2 //要求退出

//单个文件播放返回码
#define PFRC_NOERR    0 //正常结束
#define PFRC_FMTERR   1 //格式错误
#define PFRC_EXIT     2 //要求退出

FRESULT wp_rc;                                       /* Result code */
DIRS wp_dir;                                         /* Directory object */
FILINFO wp_fno;                                      /* File information object */
FIL wp_fil;                                          /* File object */

#define WP_BUF_SIZE 256
char wp_buf[WP_BUF_SIZE];

#define MAX_VOLUME 16
#define MAX_VOLUME_BIT 4
uint8_t curVolume = 1; //初始音量

uint8_t skipSample = 1; //采样率过高时降低输出采样率，表示每几个采样点中抽一个输出

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
          continue; //检查文件扩展名

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
  etft_DisplayString(wp_buf, 0, 48, 0xFFFF, 0); //显示声道数
  sprintf(wp_buf, "SamplesPerSec = %5ld     ", audio_wav_fmt.dwSamplesPerSec);
  etft_DisplayString(wp_buf, 0, 64, 0xFFFF, 0); //显示采样率
  sprintf(wp_buf, "BitsPerSample = %2d     ", audio_wav_fmt.wBitsPerSample);
  etft_DisplayString(wp_buf, 0, 80, 0xFFFF, 0); //显示每个采样的比特数

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
  etft_DisplayString(wp_buf, 0, 32, 0xFFFF, 0); //显示当前秒数
}

void wp_DisplayVolume()
{
  sprintf(wp_buf, "Volume = %02d    ", curVolume);
  etft_DisplayString(wp_buf, 0, 96, 0xFFFF, 0); //显示当前音量
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
  wp_rc = f_open(&wp_fil, filename, FA_READ); //打开文件

  wp_DisplayVolume();

  sprintf(wp_buf, "%-30s", wp_fno.fname);
  etft_DisplayString(wp_buf, 0, 16, 0xFFFF, 0);
  etft_DisplayString("S7 : Vol-      S3 : Vol+      ", 320 - 240, 240 - 32, 0xFFFF, 0);
  etft_DisplayString("S5 : Next      S4 : Exit      ", 320 - 240, 240 - 16, 0xFFFF, 0);

  //读取RIFF文件头
  UINT size;
  wp_rc = f_read(&wp_fil, &audio_riff_header, sizeof(audio_riff_header), &size); /* Read a chunk of file */
  if(wp_rc || (size != sizeof(audio_riff_header)))
    return PFRC_FMTERR; /* Error or end of file */
  if(!audio_CheckRIFFHeader())
    return PFRC_FMTERR; //文件头校验失败

  int fmtReaded = 0;
  while(1) //读取后续CHUNK
  {
    wp_rc = f_read(&wp_fil, &curChunkHeader, sizeof(curChunkHeader), &size); /* Read a chunk of file */
    if(wp_rc || (size != sizeof(curChunkHeader)))
      return PFRC_FMTERR; /* Error or end of file */

    if(memcmp(curChunkHeader.szID, "fmt ", 4) == 0) //格式CHUNK
    {
      fmtReaded = 1;
      wp_rc = f_read(&wp_fil, &audio_wav_fmt, curChunkHeader.dwChunkSize, &size); /* Read a chunk of file */
      if(wp_rc || (size != curChunkHeader.dwChunkSize))
        return PFRC_FMTERR; /* Error or end of file */
      if(!audio_CheckWavFmt())
        return PFRC_FMTERR; //格式不支持
    }
    else if(memcmp(curChunkHeader.szID, "data", 4) == 0) //已经到data段
    {
      break; //普通CHUNK读取结束，跳出while开始解析data
    }
    else //其余CHUNK不解析
    {
      wp_rc = f_lseek(&wp_fil, wp_fil.fptr + curChunkHeader.dwChunkSize);
      if(wp_rc) return PFRC_FMTERR; //文件大小有问题
    }
  }

  if(!fmtReaded)
    return PFRC_FMTERR; //缺少fmt段

  uint32_t curDataPtr = 0; //当前播放了data段的多长内容
  uint32_t sampleCnt = 0; //上一次整秒数后有多少个采样
  uint16_t curSec = 0; //当前播放的整秒数
  uint16_t fileSec = curChunkHeader.dwChunkSize / audio_wav_fmt.dwAvgBytesPerSec; //全文件秒数
  uint16_t decodedSampleCount = WP_BUF_SIZE / audio_wav_fmt.wBlockAlign / skipSample; //每次解析出的采样数
  int skipCnter = 1;

  wp_DisplayPlayTime(curSec, fileSec); //初始显示
  while(curDataPtr < curChunkHeader.dwChunkSize)
  {
    while(decodedSampleCount > audio_OutputBufferSpace()) //此时等待缓冲区有空间再继续
    {
      int rc = wp_ProcessBtns();
      if(rc == BTNRC_NEXT)
      {
        f_close(&wp_fil); //关闭文件
        return PFRC_NOERR;
      }
      if(rc == BTNRC_EXIT)
      {
        f_close(&wp_fil); //关闭文件
        return PFRC_EXIT;
      }
    }

    wp_rc = f_read(&wp_fil, &wp_buf, WP_BUF_SIZE, &size); /* Read a chunk of file */
    if(wp_rc || (size <= 0))
      break; //播放完成

    curDataPtr += size;
    uint8_t* temp = (uint8_t*)wp_buf;
    uint16_t vol[2];
    while(size > 0)
    {
      if(skipCnter >= skipSample)
      {
        skipCnter = 1;
        //不管实际声道数目，都按两个声道计算先，不需要的部分不会影响输出
        if(audio_wav_fmt.wBitsPerSample == 8)
        { //每声道8位
          vol[0] = (temp[0] << 4) & 0x0FF0;
          vol[1] = (temp[1] << 4) & 0x0FF0;
        }
        else //每声道16位
        {
          vol[0] = temp[0] | (temp[1] << 8);
          vol[0] += 0x8000; //16位是为有符号数，需要附加偏置变为无符号数
          vol[0] = (vol[0] >> 4) & 0x0FFF;
          vol[1] = temp[2] | (temp[3] << 8);
          vol[1] += 0x8000; //附加偏置
          vol[1] = (vol[1] >> 4) & 0x0FFF;
        }

        if(audio_wav_fmt.wChannels == 2)
          vol[0] = (vol[0] + vol[1]) >> 1; //若为双声道，则取平均

        vol[0] = (vol[0] * curVolume) >> MAX_VOLUME_BIT; //音量调整
        audio_Output(vol[0]); //设定DA输出
      }
      else
        skipCnter++;

      size -= audio_wav_fmt.wBlockAlign;
      temp += audio_wav_fmt.wBlockAlign;
      sampleCnt++; //采样数自增
    }

    if(sampleCnt > audio_wav_fmt.dwSamplesPerSec) //满1s
    {
      curSec++;
      sampleCnt -= audio_wav_fmt.dwSamplesPerSec;
      wp_DisplayPlayTime(curSec, fileSec);
    }
  }

  f_close(&wp_fil); //关闭文件
  return PFRC_NOERR;
}
