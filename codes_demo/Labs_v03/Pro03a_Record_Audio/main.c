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

//全局参考时钟
volatile int32_t refClock = 0;

//SD卡文件系统
FATFS fatfs;

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
  UCSCTL2 = MCLK_FREQ / (XT2CLK_FREQ / 16); //XT2频率较高，分频后作为基准可获得更高的精度
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2进行16分频后作为基准
  while (SFRIFG1 & OFIFG) { //等待XT1、XT2与DCO稳定
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG);
    SFRIFG1 &= ~OFIFG;
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //设定几个CLK的分频
  UCSCTL4 = SELA__XT1CLK + SELS__DCOCLK + SELM__DCOCLK; //设定几个CLK的时钟源

  /* 初始化参考时钟 */
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
