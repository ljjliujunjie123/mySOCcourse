#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include "dr_i2c.h"
#include "dr_tft.h"

#define TEMP_ADDR 0x2A
#define TEMP_LOCAL 0x00
#define TEMP_REMOTE 0x01
#define TEMP_CONFIG1 0x09
#define TEMP_CONFIG2 0x0A
#define TEMP_NCORR   0x21

#define BATT_ADDR 0x55
#define BATT_VOLTAGE 0x04
#define BATT_CURRENT 0x10
#define BATT_SOC 0x1C
#define BATT_CAPA 0x0C
#define BATT_FLAG 0x06

int TEMP_LOCAL_INDEX;
int TEMP_REMOTE_INDEX;
int BATT_VOLTAGE_INDEX;
int BATT_CURRENT_INDEX;
int BATT_SOC_INDEX;
int BATT_CAPA_INDEX;
int BATT_FLAG_INDEX;

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

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  _DINT();

  initClock();
  initI2C();
  initTFT();

  I2C_RequestSend(TEMP_ADDR, TEMP_CONFIG1, BIT2);
  I2C_RequestSend(TEMP_ADDR, TEMP_NCORR, 3);

  TEMP_LOCAL_INDEX = I2C_AddRegQuery(TEMP_ADDR, TEMP_LOCAL);
  TEMP_REMOTE_INDEX = I2C_AddRegQuery(TEMP_ADDR, TEMP_REMOTE);
  BATT_VOLTAGE_INDEX = I2C_AddRegQuery(BATT_ADDR, BATT_VOLTAGE);
  BATT_CURRENT_INDEX = I2C_AddRegQuery(BATT_ADDR, BATT_CURRENT);
  BATT_CAPA_INDEX = I2C_AddRegQuery(BATT_ADDR, BATT_CAPA);
  BATT_SOC_INDEX = I2C_AddRegQuery(BATT_ADDR, BATT_SOC);
  BATT_FLAG_INDEX = I2C_AddRegQuery(BATT_ADDR, BATT_FLAG);

  _EINT();

  etft_AreaSet(0,0,319,239,0);

  //充电芯片BQ24230充电指示
  P3DIR |= BIT2;
  P3OUT &= ~BIT2;

  //电池管理芯片BQ27410电池插入指示
  I2C_RequestSend(BATT_ADDR, 0x00, 0x0C);
  I2C_RequestSend(BATT_ADDR, 0x01, 0x00);

  while(1)
  {
    int temp1, temp2;
    char buffer[41];
    if(I2C_QueryHasNew(TEMP_LOCAL_INDEX))
    {
      temp1 = I2C_CheckQuery(TEMP_LOCAL_INDEX);
      temp2 = temp1 & 0xFF00;
      temp2 >>= 8;
      temp2 &= 0xFF;
      temp1 &= 0xFF;
      temp1 -= 64;
      temp2 = (temp2 >> 4) & 0xFF;
      temp2 = temp2 * 625 / 100;

      static int count;
      sprintf(buffer, "LocalTemp%d =%d.%dC", count++ & 0x07, temp1, temp2);
      etft_DisplayString(buffer, 0, 0, 0xFFFF, 0x0000);
    }

    if(I2C_QueryHasNew(TEMP_REMOTE_INDEX))
    {
      temp1 = I2C_CheckQuery(TEMP_REMOTE_INDEX);
      temp2 = temp1 & 0xFF00;
      temp2 >>= 8;
      temp2 &= 0xFF;
      temp1 &= 0xFF;
      temp1 -= 64;
      temp2 = (temp2 >> 4) & 0xFF;
      temp2 = temp2 * 625 / 100;

      static int count;
      sprintf(buffer, "RemoteTemp%d =%d.%dC", count++ & 0x07, temp1, temp2);
      etft_DisplayString(buffer, 0, 16, 0xFFFF, 0x0000);
    }

    if(I2C_QueryHasNew(BATT_VOLTAGE_INDEX))
    {
      temp1 = I2C_CheckQuery(BATT_VOLTAGE_INDEX);

      static int count;
      sprintf(buffer, "BattVoltage%d = %dmV", count++ & 0x07,  temp1);
      etft_DisplayString(buffer, 0, 32, 0xFFFF, 0x0000);
    }

    if(I2C_QueryHasNew(BATT_CURRENT_INDEX))
    {
      temp1 = I2C_CheckQuery(BATT_CURRENT_INDEX);

      static int count;
      sprintf(buffer, "BattCurrent%d = %dmA", count++ & 0x07,  temp1);
      etft_DisplayString(buffer, 0, 48, 0xFFFF, 0x0000);
    }

    if(I2C_QueryHasNew(BATT_SOC_INDEX))
    {
      temp1 = I2C_CheckQuery(BATT_SOC_INDEX);

      static int count;
      sprintf(buffer, "BattSoC%d = %d%%", count++ & 0x07,  temp1);
      etft_DisplayString(buffer, 0, 64, 0xFFFF, 0x0000);
    }

    if(I2C_QueryHasNew(BATT_CAPA_INDEX))
    {
      temp1 = I2C_CheckQuery(BATT_CAPA_INDEX);

      static int count;
      sprintf(buffer, "BattCap%d = %dmAh", count++ & 0x07,  temp1);
      etft_DisplayString(buffer, 0, 80, 0xFFFF, 0x0000);
    }

    if(I2C_QueryHasNew(BATT_FLAG_INDEX))
    {
      temp1 = I2C_CheckQuery(BATT_FLAG_INDEX);

      static int count;
      sprintf(buffer, "BattFlag%d = 0x%x", count++ & 0x07,  temp1);
      etft_DisplayString(buffer, 0, 96, 0xFFFF, 0x0000);
    }

    __delay_cycles(2000000);
  }
}
