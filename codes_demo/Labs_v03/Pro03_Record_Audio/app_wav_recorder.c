#include <msp430.h>
#include "app_wav_recorder.h"
#include <stdint.h>
#include "dr_sdcard.h"
#include "dr_tft.h"
#include <stdio.h>
#include "dr_buttons_and_leds.h"
#include "dr_audio_io.h"

#define WR_BUF_LENGTH 256
char wr_buf[WR_BUF_LENGTH];

//RECxxx.WAV
uint8_t wr_file_no = 0;
//是否将录音立即回送到音频输出的开关
int wr_echo_switch = 1;

FRESULT wr_rc;                                       /* Result code */
DIRS wr_dir;                                         /* Directory object */
FILINFO wr_fno;                                      /* File information object */
FIL wr_fil;                                          /* File object */

//WAV文件头
const char WR_WAV_HEADER[] =
{
  'R', 'I', 'F', 'F',
  0x00, 0x00, 0x00, 0x00, //file size without 'RIFF'
  'W', 'A', 'V', 'E',
  'f', 'm', 't', ' ',
  16, 0, 0, 0, //fmt size
  1, 0, //编码方式
  1, 0, //单声道
  0x40, 0x1F, 0x00, 0x00, //8 KHz
  0x40, 0x1F, 0x00, 0x00, //8 KB/s
  1, 0, //数据对齐
  8, 0, //8 bit/sample
  'd', 'a', 't', 'a',
  0x00, 0x00, 0x00, 0x00, //data size
};

void wr_TryAutoSelectFileName()
{
  int i;
  for(i=0;i<255;++i)
  {
    sprintf(wr_buf, "REC%03d.WAV", i);
    wr_rc = f_stat(wr_buf, &wr_fno);
    if(wr_rc) //文件不存在
    {
      wr_file_no = i;
      return;
    }
  }
  wr_file_no = 255;
}

void app_StartWavRecorder()
{
  etft_AreaSet(0,0,319,239,0);
  etft_DisplayString("Audio Recorder", 104, 0, etft_Color(0, 255, 0), 0);
  etft_DisplayString("Create/overwrite a wav file:", 0, 16, 0xFFFF, 0);
  etft_DisplayString("S5 : Confirm   S6 : Auto choose", 0, 240 - 48, 0xFFFF, 0);
  etft_DisplayString("S7 : Previous  S3 : Next", 0, 240 - 32, 0xFFFF, 0);
  etft_DisplayString("               S4 : Exit", 0, 240 - 16, 0xFFFF, 0);

  while(1)
  {
    sprintf(wr_buf, "REC%03u.WAV", wr_file_no);
    etft_DisplayString(wr_buf, 0, 32, 0xFFFF, 0);

    wr_rc = f_stat(wr_buf, &wr_fno);
    if(wr_rc) //文件不存在
      etft_DisplayString(" - Not exist    ", 10 * 8, 32, 0xFFFF, 0);
    else
      etft_DisplayString(" - Already exist", 10 * 8, 32, 0xFFFF, 0);

    int key = btn_GetBtnInput(1);
    switch(key)
    {
    case 3:
      wr_file_no++;
      break;
    case 7:
      wr_file_no--;
      break;

    case 5:
      app_RecordAudio(wr_buf);
      return;

    case 6:
      wr_TryAutoSelectFileName();
      break;

    case 4:
    default:
      return;
    }
  }
}

void wr_DisplayRecordState(uint32_t sampleCount, int forceUpdate)
{
  static char dotBuff[] = "...   ";
  static char strBuff[41];
  static uint16_t secCount;
  static uint32_t lastSampleCnt = 0;

  if(sampleCount <= lastSampleCnt) //计数复位
  {
    secCount = 0;
    lastSampleCnt = 0;
    forceUpdate = 1;
  }

  while(sampleCount - lastSampleCnt > RECORDER_SAMPLE_RATE) //新采样已超过1s
  {
    secCount++;
    lastSampleCnt += RECORDER_SAMPLE_RATE;
    forceUpdate = 1;
  }

  if(forceUpdate)
  {
    etft_DisplayString(dotBuff + 3 - (secCount & 0x3), 9*8, 32, etft_Color(0xFF,0,0), 0);
    sprintf(strBuff, "%02d:%02d", secCount / 60, secCount % 60);
    etft_DisplayString(strBuff, 7*8, 48, 0xFFFF, 0);
    sprintf(strBuff, "%ld      ", sampleCount + 40);
    etft_DisplayString(strBuff, 7*8, 64, 0xFFFF, 0);
    sprintf(strBuff, "%d", wr_echo_switch);
    etft_DisplayString(strBuff, 7*8, 80, 0xFFFF, 0);
  }
}

void app_RecordAudio(const char* filename)
{
  uint32_t sampleCnt = 0;
  UINT size;
  etft_AreaSet(0,16,319,239,0);
  wr_rc = f_open(&wr_fil, filename, FA_WRITE + FA_CREATE_ALWAYS);
  if(wr_rc)
  {
    etft_DisplayString("File create failed, press any key to exit...", 0, 16, 0xFFFF, 0);
    btn_GetBtnInput(1);
    return;
  }

  f_write(&wr_fil, WR_WAV_HEADER, sizeof(WR_WAV_HEADER), &size);

  etft_DisplayString(filename, 0, 16, 0xFFFF, 0);
  etft_DisplayString("Recording", 0, 32, etft_Color(0xFF,0,0), 0);
  etft_DisplayString("Time = ", 0, 48, 0xFFFF, 0);
  etft_DisplayString("Size = ", 0, 64, 0xFFFF, 0);
  etft_DisplayString("Echo = ", 0, 80, 0xFFFF, 0);
  wr_DisplayRecordState(sampleCnt, 1);
  etft_DisplayString("S4 : Echo switch", 10 * 8, 240 - 32, 0xFFFF, 0);
  etft_DisplayString("S5 : Save and exit", 10 * 8, 240 - 16, 0xFFFF, 0);

  audio_SetSampleRate(RECORDER_SAMPLE_RATE);
  audio_InputClear(); //清空之前的采样结果，从当前时间开始采样

  while(1)
  {
    size = audio_InputRead((uint8_t*)wr_buf, WR_BUF_LENGTH);
    if(size > 0)
    {
      sampleCnt += size;
      f_write(&wr_fil, wr_buf, size, &size);
      wr_DisplayRecordState(sampleCnt, 0);

      if(wr_echo_switch)
      {
        int i;
        for(i=0;i<size;++i)
          audio_Output(wr_buf[i] << 4);
      }
    }



    if(btn_Pressed(4))
    {
      wr_echo_switch = !wr_echo_switch;
      wr_DisplayRecordState(sampleCnt, 1);
    }
    if(btn_Pressed(5))
      break;
  }

  f_lseek(&wr_fil, 0x28); //data size
  f_write(&wr_fil, &sampleCnt, 4, &size);
  sampleCnt += 40;
  f_lseek(&wr_fil, 0x04); //file size
  f_write(&wr_fil, &sampleCnt, 4, &size);

  f_close(&wr_fil);

  etft_AreaSet(0,32,319,239,0);
  etft_DisplayString("Saved", 0, 32, etft_Color(0,0xFF,0), 0);
  etft_DisplayString("Press any key to exit...", 0, 48, 0xFFFF, 0);
  btn_GetBtnInput(1);
  return;
}
