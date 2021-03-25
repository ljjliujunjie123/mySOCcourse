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
        	  usbmsc_IdleTask();                            //SD����д����
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
