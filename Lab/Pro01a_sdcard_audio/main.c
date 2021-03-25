#include <msp430.h>
#include "dr_sdcard.h"
#include "dr_audio.h"
#include "fw_queue.h"
#include "dr_tft.h"
#include "dr_usbmsc.h"
#include <string.h>
#include <stdio.h>

void initClock()
{
  while(BAKCTL & LOCKIO) // Unlock XT1 pins for operation
    BAKCTL &= ~(LOCKIO);
  UCSCTL6 &= ~XT1OFF; //启动XT1
  P7SEL |= BIT2 + BIT3; //XT2引脚功能选择
  UCSCTL6 &= ~XT2OFF; //启动XT2
  while (SFRIFG1 & OFIFG) { //等待XT1、XT2与DCO稳定
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG); 
    SFRIFG1 &= ~OFIFG; 
  }
  
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__XT2CLK; //避免DCO调整中跑飞
  
  UCSCTL1 = DCORSEL_5; //6000kHz~23.7MHz
  UCSCTL2 = 20000000 / (4000000 / 16); //XT2频率较高，分频后作为基准可获得更高的精度
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2进行16分频后作为基准
  while (SFRIFG1 & OFIFG) { //等待XT1、XT2与DCO稳定
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG); 
    SFRIFG1 &= ~OFIFG; 
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //设定几个CLK的分频
  UCSCTL4 = SELA__XT1CLK + SELS__DCOCLK + SELM__DCOCLK; //设定几个CLK的时钟源
}

#define BUFFER_SIZE 4096
uint8_t buffer[BUFFER_SIZE];

Queue wav_stream;

FRESULT rc;                                       /* Result code */
FATFS fatfs;                                      /* File system object */
DIRS dir;                                         /* Directory object */
FILINFO fno;                                      /* File information object */
FIL fil;                                          /* File object */

#define FBUF_SIZE 256
char fbuf[FBUF_SIZE];

void strupr(char* str)
{
  char *ptr = str;
  while(!*ptr)
  {
    if(*ptr >= 'a' && *ptr <='z')
      *ptr -= 'a' - 'A';
    ptr++;
  }
}

int __low_level_init(void)
{
  WDTCTL = WDTPW + WDTHOLD ;//stop wdt
  return 1;
}

void idleTasks()
{
  usbmsc_IdleTask(); //处理U盘空闲时任务
                     //对于本程序，由于其他代码的存在，调用频率可以保证不会过高
}

//#include "app_icon.h"

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;
  
  _DINT();
  initClock();
  initAudio();
  initTFT();
  initUSBMSC();
  _EINT();
  
  initQueue(&wav_stream, buffer, BUFFER_SIZE); //初始化音乐播放使用的音频缓冲区
  audio_SetWavStream(&wav_stream); //设定解码模块从指定缓冲区中获取需要解码的数据流

  etft_AreaSet(0,0,319,239,0);
  
  //按键IO初始化
  P4REN |= 0x1F; //使能按键端口上的上下拉电阻
  P4OUT |= 0x1F; //上拉状态
  uint8_t last_btn = 0x1F, cur_btn, temp;
  //LED IO
  P4DIR |= BIT5 + BIT6;
  P4OUT &= ~(BIT5 + BIT6);
  
  //etft_DisplayImage(image, 319 - 128, 239 - 96, 128, 96);

  while(1)
  {
    f_mount(0, &fatfs); /* Register volume work area (never fails) */
    rc = f_opendir(&dir, "");
    if(rc)
    {
      etft_DisplayString("ERROR: No SDCard detected!", 0, 0, etft_Color(0xFF,0,0), 0);
    }

    for (;;)
    {
      idleTasks(); //保证即使没有文件，也可以处理USBMSC事务
      rc = f_readdir(&dir, &fno);                        // Read a directory item
      if (rc || !fno.fname[0]) break;                    // Error or end of dir
      if (fno.fattrib & AM_DIR)                          //this is a directory
      {
        
      }
      else                                               //this is a file
      {
        strcpy(fbuf, fno.fname);
        strupr(fbuf);
        
        if(strstr(fbuf, ".WAV") == 0)
          continue; //检查文件扩展名
        
        rc = f_open(&fil, fno.fname, FA_READ); //打开文件
        
        char display[41];
        sprintf(display, "%-30s", fno.fname);
        etft_DisplayString(display, 0, 0, 0xFFFF, 0);
        int flag = 0; //基本信息是否已经显示完
        uint16_t lastSec = 0xFFFF;
        
        for (;;)
        {
          uint16_t wavsize = queue_CanWrite(&wav_stream); //检查音频缓冲区中还有多少空间
          uint16_t size = FBUF_SIZE < wavsize ? FBUF_SIZE : wavsize; //取音频缓冲区和文件读取缓冲区剩余空间中较小者
          
          if(wavsize > FBUF_SIZE) //队列更空，SD速率不够，此时不执行其它空闲时任务
          {
            P4OUT |= BIT5; //LED5
            P4OUT &= ~BIT6;
          }
          else //SD速率足够
          {
            P4OUT &= ~BIT5; 
            P4OUT |= BIT6; //LED4
            idleTasks(); //仅在缓冲区内容充足时处理其它空闲时任务
          }
          
          if(size > 0)
          {
            rc = f_read(&fil, fbuf, size, &size);        /* Read a chunk of file */
            if (rc || !size) break;                      /* Error or end of file */
            queue_Write(&wav_stream, (uint8_t*)fbuf, size); // 将读出的数据放入音频缓冲区
          }
          
          if(wavsize <= FBUF_SIZE) //缓冲区数据充足，有空处理显示
          {
            const AudioDecoderStatus *ads = audio_DecoderStatus(); //获取解码器状态
            if(ads->wFlags & ADI_INVALID_FILE) //此文件有错，跳出并结束播放
              break;

            if(ads->wFlags & ADI_PLAYING) //真正播放中，可以读出文件播放状态
            {
              if(lastSec != ads->wCurPosSec) //播放秒值有更新，需要刷新显示
              {
                sprintf(display, "%02d:%02d/%02d:%02d", ads->wCurPosSec / 60
                        , ads->wCurPosSec % 60, ads->wFullSec / 60, ads->wFullSec % 60);
                etft_DisplayString(display, 0, 16, 0xFFFF, 0); //显示当前秒数
                lastSec = ads->wCurPosSec;

                sprintf(display, "SampleLost = %5ld", ads->dwLostCount);
                etft_DisplayString(display, 0, 80, 0xFFFF, 0); //显示采样丢失数，这一数目反映了其它空闲时任务是否已经干扰到播放的进行
                                                               //但若是音频播放的定时中断丢失，则不能被这一参数反映
                cur_btn = P4IN & 0x1F;
                temp = (cur_btn ^ last_btn) & last_btn; //找出刚向下跳变的按键
                last_btn = cur_btn;
                if(temp & BIT2) //S5
                {
                  break; //下一曲
                }
                else if(temp & BIT0) //S7
                {
                  audio_SetVolume(1); //调节音量到最小
                }
                else if(temp & BIT4) //S3
                {
                  audio_SetVolume(16); //调节音量到最大
                }
              }
              if(flag == 0) //在播放新文件后还未更新文件基本信息
              {
                flag = 1;
                sprintf(display, "Channels = %d     ", ads->wChannels);
                etft_DisplayString(display, 0, 32, 0xFFFF, 0); //显示声道数
                sprintf(display, "SamplesPerSec = %5ld     ", ads->dwSamplesPerSec);
                etft_DisplayString(display, 0, 48, 0xFFFF, 0); //显示采样率
                sprintf(display, "BitsPerSample = %2d     ", ads->wBitsPerSample);
                etft_DisplayString(display, 0, 64, 0xFFFF, 0); //显示每个采样的比特数
              }
            }
          }
        }
        
        audio_DecoderReset(); //复位解码器
        f_close(&fil); //关闭文件
      }
    }
  }
}
