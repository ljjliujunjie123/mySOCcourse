#include <msp430.h>
#include "dr_sdcard.h"
#include "dr_tft.h"
#include "dr_usbmsc.h"
#include "app_bmp.h"
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

FRESULT rc;   /* Result code */
FATFS fatfs;  /* File system object */
DIRS dir;     /* Directory object */
FILINFO fno;  /* File information object */

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
  if(usbmsc_IdleTask()) //此时刚处理了读写操作
    __delay_cycles(20000000 / 1000); //延时避免查询过于频繁导致USB模块卡死
}

#include "app_icon.h"

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  _DINT();
  initClock();
  initTFT();
  initUSBMSC();
  _EINT();

  etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));

  //按键IO初始化
  P4REN |= 0x1F; //使能按键端口上的上下拉电阻
  P4OUT |= 0x1F; //上拉状态
  uint8_t last_btn = 0x1F, cur_btn, temp;
  //LED IO
  P4DIR |= BIT5 + BIT6;
  P4OUT &= ~(BIT5 + BIT6);

  etft_DisplayImage(icon, 0, 0, 30, 30);
  etft_DisplayString("BMP Viewer", 31, 0, etft_Color(0, 0, 255), etft_Color(128, 128, 128));

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
        char temps[20];
        strcpy(temps, fno.fname);
        strupr(temps);

        if(strstr(temps, ".BMP") == 0)
          continue; //检查文件扩展名

        etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));
        if(bmp_DisplayBmpFile(fno.fname)) //文件解析显示成功
        {

        }
        else
        {
          etft_DisplayImage(icon, 0, 0, 30, 30);
          etft_DisplayString("File not supported!", 31, 0, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
          etft_DisplayString(fno.fname, 31, 16, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
        }

        while(1)
        {
          idleTasks();
          cur_btn = P4IN & 0x1F;
          temp = (cur_btn ^ last_btn) & last_btn; //找出刚向下跳变的按键
          last_btn = cur_btn;
          if(temp & BIT2) //S5
            break;
        }
      }
    }
  }
}
