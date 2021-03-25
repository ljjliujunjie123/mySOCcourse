#include "include.h"

#define ALL_PORT (BIT1+BIT2+BIT3+BIT4+BIT5)
#define OSC_CYCLES 10

void initCapTouch()
{
  //Vref加到+极
  CBCTL2 = CBREF14+CBREF13+CBREF02;
  CBCTL1 = CBON + CBF;
  CBCTL2 |= CBRS_1;
  CBCTL3 = ALL_PORT;
}

uint16_t CapTouch_ReadChannel(int i)
{
  uint16_t cpu_cnt = 0;
  uint16_t osc_cnt = 0;

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

void touchkeymain(void)
{
    uint32_t temp[5] ={0,0,0,0,0};
    int i;
    __delay_cycles(2000000);
    etft_AreaSet(0,0,319,239,0);
    etft_DisplayString("Touch any key to Lighting LED",40,100,etft_Color(0, 255, 0),0);
    etft_DisplayString("Press     to Exit",70,140,etft_Color(0, 255, 0),0);
    etft_DisplayString("S5",122,140,etft_Color(255, 0, 0),0);
    _DINT();
    ADC12IE &= ~BIT0;
    initCapTouch();

    for(;;)
    {
    	for(i=5;i>=1;i--)
    	{

    		temp[i]= CapTouch_ReadChannel(i);
    		if(temp[i]<captouch_max)
    			{
    			led_SetLed(6-i, 0); //翻转对应的LED
    			}
    		else
    			led_SetLed(6-i, 1);
    	}
//    	__delay_cycles(8000000);
    	if (btn_GetBtnInput(0) == 5)
    	{
    		//etft_AreaSet(0,0,319,239,0);
    		__delay_cycles(2000000);
    		for(i=5;i>=1;i--)
    			led_SetLed(6-i, 0);
    		ADC12IE |= BIT0;
    		  _EINT();
    		break;
    	}
    	__delay_cycles(200000);
    }
}


