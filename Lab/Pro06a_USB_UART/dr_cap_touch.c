#include <msp430.h>
#include "dr_cap_touch.h"
#include "fw_public.h"

//触摸盘在P6.1~P6.5
#define ALL_PORT (BIT1+BIT2+BIT3+BIT4+BIT5)

void initCapTouch()
{
  //Vref加到+极
  CBCTL2 = CBREF14+CBREF13+CBREF02;
  CBCTL1 = CBON + CBF;
  CBCTL2 |= CBRS_1;
  CBCTL3 = ALL_PORT;
}

#define OSC_CYCLES 10

uint16_t CapTouch_ReadChannel(int i)
{
  uint16_t cpu_cnt = 0;
  uint16_t osc_cnt = 0;

  if(i < 1) i = 1;
  if(i > 5) i = 5;

  CBCTL0 = CBIMEN + (i << 8); //外部信号加到负极
  P6OUT &= ~ALL_PORT;
  P6DIR |= ALL_PORT & ~(1 << i);
  CBCTL3 = 1 << i;

  uint16_t gie;
  _ECRIT(gie);

  while(1)
  {
    if(CBCTL1 & CBOUT)
      P6OUT |= ALL_PORT;
    else
      P6OUT &= ~ALL_PORT;

    if(CBINT & CBIFG)
    {
      CBINT &= ~CBIFG;
      osc_cnt++;
    }

    if(osc_cnt >= OSC_CYCLES)
      break;

    cpu_cnt++;
  }

  _LCRIT(gie);

  CBCTL3 = ALL_PORT;
  P6DIR &= ~ALL_PORT;

  return cpu_cnt;
}
