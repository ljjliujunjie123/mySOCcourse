#include <msp430.h>
#include <string.h>
#include <stdio.h>
#include "dr_tft.h"
#include "dr_buttons_and_leds.h"
#include "dr_audio_io.h"
#include "dr_sdcard.h"
#include "app_wav_player.h"
#include "app_wav_recorder.h"
#include "dr_usbmsc.h"

#define MCLK_FREQ   20000000
#define SMCLK_FREQ  20000000
#define XT2CLK_FREQ  4000000
#define REFCLK_FREQ     1000

//ȫ�ֲο�ʱ��
volatile int32_t refClock = 0;

//SD���ļ�ϵͳ
FATFS fatfs;

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
  UCSCTL2 = MCLK_FREQ / (XT2CLK_FREQ / 16); //XT2Ƶ�ʽϸߣ���Ƶ����Ϊ��׼�ɻ�ø��ߵľ���
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2����16��Ƶ����Ϊ��׼
  while (SFRIFG1 & OFIFG) { //�ȴ�XT1��XT2��DCO�ȶ�
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //�趨����CLK�ķ�Ƶ
  UCSCTL4 = SELA__XT1CLK + SELS__DCOCLK + SELM__DCOCLK; //�趨����CLK��ʱ��Դ

  /* ��ʼ���ο�ʱ�� */
  TA2CTL = TASSEL__SMCLK + MC__UP + ID__1;
  TA2CCR0 = (SMCLK_FREQ / REFCLK_FREQ) - 1;
  TA2CCTL0 = CCIE;
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void refClockInterrupt()
{
  refClock++;
  usbmsc_IdleTask();
}

int __low_level_init(void)
{
  WDTCTL = WDTPW + WDTHOLD ;//stop wdt
  return 1;
}

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  _DINT();
  initClock();
  initTFT();
  initButtonsAndLeds();
  initAudioIO();
  f_mount(0, &fatfs);
  initUSBMSC();
  _EINT();

  while(1)
  {
    app_StartWavRecorder();
    app_StartWavPlayer();
  }
}
