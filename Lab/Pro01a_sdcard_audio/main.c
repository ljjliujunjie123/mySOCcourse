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
  UCSCTL6 &= ~XT1OFF; //����XT1
  P7SEL |= BIT2 + BIT3; //XT2���Ź���ѡ��
  UCSCTL6 &= ~XT2OFF; //����XT2
  while (SFRIFG1 & OFIFG) { //�ȴ�XT1��XT2��DCO�ȶ�
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG); 
    SFRIFG1 &= ~OFIFG; 
  }
  
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__XT2CLK; //����DCO�������ܷ�
  
  UCSCTL1 = DCORSEL_5; //6000kHz~23.7MHz
  UCSCTL2 = 20000000 / (4000000 / 16); //XT2Ƶ�ʽϸߣ���Ƶ����Ϊ��׼�ɻ�ø��ߵľ���
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2����16��Ƶ����Ϊ��׼
  while (SFRIFG1 & OFIFG) { //�ȴ�XT1��XT2��DCO�ȶ�
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG); 
    SFRIFG1 &= ~OFIFG; 
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //�趨����CLK�ķ�Ƶ
  UCSCTL4 = SELA__XT1CLK + SELS__DCOCLK + SELM__DCOCLK; //�趨����CLK��ʱ��Դ
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
  usbmsc_IdleTask(); //����U�̿���ʱ����
                     //���ڱ�����������������Ĵ��ڣ�����Ƶ�ʿ��Ա�֤�������
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
  
  initQueue(&wav_stream, buffer, BUFFER_SIZE); //��ʼ�����ֲ���ʹ�õ���Ƶ������
  audio_SetWavStream(&wav_stream); //�趨����ģ���ָ���������л�ȡ��Ҫ�����������

  etft_AreaSet(0,0,319,239,0);
  
  //����IO��ʼ��
  P4REN |= 0x1F; //ʹ�ܰ����˿��ϵ�����������
  P4OUT |= 0x1F; //����״̬
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
      idleTasks(); //��֤��ʹû���ļ���Ҳ���Դ���USBMSC����
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
          continue; //����ļ���չ��
        
        rc = f_open(&fil, fno.fname, FA_READ); //���ļ�
        
        char display[41];
        sprintf(display, "%-30s", fno.fname);
        etft_DisplayString(display, 0, 0, 0xFFFF, 0);
        int flag = 0; //������Ϣ�Ƿ��Ѿ���ʾ��
        uint16_t lastSec = 0xFFFF;
        
        for (;;)
        {
          uint16_t wavsize = queue_CanWrite(&wav_stream); //�����Ƶ�������л��ж��ٿռ�
          uint16_t size = FBUF_SIZE < wavsize ? FBUF_SIZE : wavsize; //ȡ��Ƶ���������ļ���ȡ������ʣ��ռ��н�С��
          
          if(wavsize > FBUF_SIZE) //���и��գ�SD���ʲ�������ʱ��ִ����������ʱ����
          {
            P4OUT |= BIT5; //LED5
            P4OUT &= ~BIT6;
          }
          else //SD�����㹻
          {
            P4OUT &= ~BIT5; 
            P4OUT |= BIT6; //LED4
            idleTasks(); //���ڻ��������ݳ���ʱ������������ʱ����
          }
          
          if(size > 0)
          {
            rc = f_read(&fil, fbuf, size, &size);        /* Read a chunk of file */
            if (rc || !size) break;                      /* Error or end of file */
            queue_Write(&wav_stream, (uint8_t*)fbuf, size); // �����������ݷ�����Ƶ������
          }
          
          if(wavsize <= FBUF_SIZE) //���������ݳ��㣬�пմ�����ʾ
          {
            const AudioDecoderStatus *ads = audio_DecoderStatus(); //��ȡ������״̬
            if(ads->wFlags & ADI_INVALID_FILE) //���ļ��д���������������
              break;

            if(ads->wFlags & ADI_PLAYING) //���������У����Զ����ļ�����״̬
            {
              if(lastSec != ads->wCurPosSec) //������ֵ�и��£���Ҫˢ����ʾ
              {
                sprintf(display, "%02d:%02d/%02d:%02d", ads->wCurPosSec / 60
                        , ads->wCurPosSec % 60, ads->wFullSec / 60, ads->wFullSec % 60);
                etft_DisplayString(display, 0, 16, 0xFFFF, 0); //��ʾ��ǰ����
                lastSec = ads->wCurPosSec;

                sprintf(display, "SampleLost = %5ld", ads->dwLostCount);
                etft_DisplayString(display, 0, 80, 0xFFFF, 0); //��ʾ������ʧ������һ��Ŀ��ӳ����������ʱ�����Ƿ��Ѿ����ŵ����ŵĽ���
                                                               //��������Ƶ���ŵĶ�ʱ�ж϶�ʧ�����ܱ���һ������ӳ
                cur_btn = P4IN & 0x1F;
                temp = (cur_btn ^ last_btn) & last_btn; //�ҳ�����������İ���
                last_btn = cur_btn;
                if(temp & BIT2) //S5
                {
                  break; //��һ��
                }
                else if(temp & BIT0) //S7
                {
                  audio_SetVolume(1); //������������С
                }
                else if(temp & BIT4) //S3
                {
                  audio_SetVolume(16); //�������������
                }
              }
              if(flag == 0) //�ڲ������ļ���δ�����ļ�������Ϣ
              {
                flag = 1;
                sprintf(display, "Channels = %d     ", ads->wChannels);
                etft_DisplayString(display, 0, 32, 0xFFFF, 0); //��ʾ������
                sprintf(display, "SamplesPerSec = %5ld     ", ads->dwSamplesPerSec);
                etft_DisplayString(display, 0, 48, 0xFFFF, 0); //��ʾ������
                sprintf(display, "BitsPerSample = %2d     ", ads->wBitsPerSample);
                etft_DisplayString(display, 0, 64, 0xFFFF, 0); //��ʾÿ�������ı�����
              }
            }
          }
        }
        
        audio_DecoderReset(); //��λ������
        f_close(&fil); //�ر��ļ�
      }
    }
  }
}
