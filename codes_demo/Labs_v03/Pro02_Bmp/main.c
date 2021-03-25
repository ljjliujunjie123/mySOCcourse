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
  if(usbmsc_IdleTask()) //��ʱ�մ����˶�д����
    __delay_cycles(20000000 / 1000); //��ʱ�����ѯ����Ƶ������USBģ�鿨��
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

  //����IO��ʼ��
  P4REN |= 0x1F; //ʹ�ܰ����˿��ϵ�����������
  P4OUT |= 0x1F; //����״̬
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
      idleTasks(); //��֤��ʹû���ļ���Ҳ���Դ���USBMSC����
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
          continue; //����ļ���չ��

        etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));
        if(bmp_DisplayBmpFile(fno.fname)) //�ļ�������ʾ�ɹ�
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
          temp = (cur_btn ^ last_btn) & last_btn; //�ҳ�����������İ���
          last_btn = cur_btn;
          if(temp & BIT2) //S5
            break;
        }
      }
    }
  }
}
