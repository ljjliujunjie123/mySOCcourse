#include <msp430.h>
#include "dr_sdcard.h"
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
  initUSBMSC();
  _EINT();
  while (1)
  {
      switch (USB_connectionState())
      {
          case ST_USB_DISCONNECTED:
              __bis_SR_register(LPM3_bits + GIE);    // Enter LPM3 until VBUS-on event
              _NOP();
              break;

          case ST_USB_CONNECTED_NO_ENUM:
              break;

          case ST_ENUM_ACTIVE:
        	  usbmsc_IdleTask();                            //SD卡读写函数
              break;

          case ST_ENUM_SUSPENDED:
              __bis_SR_register(LPM3_bits + GIE);    // Enter LPM3, until a resume or VBUS-off
                                                     // event
              break;

          case ST_ENUM_IN_PROGRESS:
              break;

          case ST_ERROR:
              break;
          default:;
      }
  }
  return 0;
}
