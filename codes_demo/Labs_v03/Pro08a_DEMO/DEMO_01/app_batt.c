#include "app_batt.h"
#include "dr_i2c.h"

int TEMP_LOCAL_INDEX;
int TEMP_REMOTE_INDEX;
int BATT_VOLTAGE_INDEX;
int BATT_CURRENT_INDEX;
int BATT_SOC_INDEX;
int BATT_CAPA_INDEX;
int BATT_FLAG_INDEX;

void initBATT(void)
{
  I2C_RequestSend(TEMP_ADDR, TEMP_CONFIG1, BIT2);
  I2C_RequestSend(TEMP_ADDR, TEMP_NCORR, 3);

  TEMP_LOCAL_INDEX = I2C_AddRegQuery(TEMP_ADDR, TEMP_LOCAL);
  TEMP_REMOTE_INDEX = I2C_AddRegQuery(TEMP_ADDR, TEMP_REMOTE);
  BATT_VOLTAGE_INDEX = I2C_AddRegQuery(BATT_ADDR, BATT_VOLTAGE);
  BATT_CURRENT_INDEX = I2C_AddRegQuery(BATT_ADDR, BATT_CURRENT);
  BATT_CAPA_INDEX = I2C_AddRegQuery(BATT_ADDR, BATT_CAPA);
  BATT_SOC_INDEX = I2C_AddRegQuery(BATT_ADDR, BATT_SOC);
  BATT_FLAG_INDEX = I2C_AddRegQuery(BATT_ADDR, BATT_FLAG);

  //充电芯片BQ24230充电指示
  P3DIR |= BIT2;
  P3OUT &= ~BIT2;

  //电池管理芯片BQ27410电池插入指示
  I2C_RequestSend(BATT_ADDR, 0x00, 0x0C);
  I2C_RequestSend(BATT_ADDR, 0x01, 0x00);
}

void BATTstatus(void)
{
    int temp1, temp2;
    char buffer[41];
    char dot[3] = ". ";
    etft_DisplayString("Temp and Battery Monitor", 8*8, 0, etft_Color(0, 255, 0), 0x0000);

    etft_DisplayString("Temp", 0, 32, etft_Color(255, 128, 0), 0x0000);
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
      sprintf(buffer, "Local = %d.%dC %c  ", temp1, temp2, dot[count++ & 1]);
      etft_DisplayString(buffer, 40, 48, 0xFFFF, 0x0000);
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
      sprintf(buffer, "Remote = %d.%dC %c  ", temp1, temp2, dot[count++ & 1]);
      etft_DisplayString(buffer, 40, 64, 0xFFFF, 0x0000);
    }

    etft_DisplayString("Batt", 0, 6*16, etft_Color(255, 128, 0), 0x0000);
    if(I2C_QueryHasNew(BATT_VOLTAGE_INDEX))
    {
      temp1 = I2C_CheckQuery(BATT_VOLTAGE_INDEX);

      static int count;
      sprintf(buffer, "Voltage = %dmV %c  ",  temp1, dot[count++ & 1]);
      etft_DisplayString(buffer, 40, 7*16, 0xFFFF, 0x0000);
    }

    if(I2C_QueryHasNew(BATT_CURRENT_INDEX))
    {
      temp1 = I2C_CheckQuery(BATT_CURRENT_INDEX);

      static int count;
      sprintf(buffer, "Current = %dmA %c  ",  temp1, dot[count++ & 1]);
      etft_DisplayString(buffer, 40, 8*16, 0xFFFF, 0x0000);
    }

    if(I2C_QueryHasNew(BATT_SOC_INDEX))
    {
      temp1 = I2C_CheckQuery(BATT_SOC_INDEX);

      static int count;
      sprintf(buffer, "Status = %d%% %c  ",  temp1, dot[count++ & 1]);
      etft_DisplayString(buffer, 40, 9*16, 0xFFFF, 0x0000);
    }

    if(I2C_QueryHasNew(BATT_CAPA_INDEX))
    {
      temp1 = I2C_CheckQuery(BATT_CAPA_INDEX);

      static int count;
      sprintf(buffer, "Capacity = %dmAh %c  ",  temp1 * 2 / 3, dot[count++ & 1]);
      etft_DisplayString(buffer, 40, 10*16, 0xFFFF, 0x0000);
    }

    if(I2C_QueryHasNew(BATT_FLAG_INDEX))
    {
      temp1 = I2C_CheckQuery(BATT_FLAG_INDEX);

      static int count;
      sprintf(buffer, "Flag = 0x%x %c  ",  temp1, dot[count++ & 1]);
      etft_DisplayString(buffer, 40, 11*16, 0xFFFF, 0x0000);
    }
}

void BATTmain(void)
{
  __delay_cycles(2000000);
  ADC12IE &= ~BIT0;
  initI2C();
  initBATT();
  etft_AreaSet(0,0,319,239,0);
  etft_DisplayString("Press     to Exit",70,210,etft_Color(0, 255, 0),0);
  etft_DisplayString("S5",122,210,etft_Color(255, 0, 0),0);
  for(;;)
  {
    BATTstatus();
    //cur_btn = P4IN & 0x1F;
    //temp = (cur_btn ^ last_btn) & last_btn; //找出刚向下跳变的按键
    //last_btn = cur_btn;

    if (btn_GetBtnInput(0) == 5)
    {
      //etft_AreaSet(0,0,319,239,0);
      UCB0CTL1 |= UCSWRST;
      UCB0IE &= ~UCNACKIE;
      __delay_cycles(2000000);
      ADC12IE |= BIT0;
      break;
    }
    __delay_cycles(2000000);
  }
}
