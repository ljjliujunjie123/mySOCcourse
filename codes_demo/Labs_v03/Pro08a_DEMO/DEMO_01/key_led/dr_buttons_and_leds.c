#include <msp430.h>
#include <stdint.h>
#include "dr_buttons_and_leds.h"

#define LED_COUNT 5
#define BTN_COUNT 5

#ifdef REF_CLOCK
extern volatile int32_t REF_CLOCK;
#endif

typedef struct
{
  const volatile uint8_t* PxIN;
  volatile uint8_t* PxOUT;
  volatile uint8_t* PxDIR;
  volatile uint8_t* PxREN;
  volatile uint8_t* PxSEL;
} GPIO_TypeDef;

const GPIO_TypeDef GPIO4 =
{
  &P4IN, &P4OUT, &P4DIR, &P4REN, &P4SEL
};

const GPIO_TypeDef GPIO5 =
{
  &P5IN, &P5OUT, &P5DIR, &P5REN, &P5SEL
};

const GPIO_TypeDef GPIO8 =
{
  &P8IN, &P8OUT, &P8DIR, &P8REN, &P8SEL
};

const GPIO_TypeDef* LED_GPIO[LED_COUNT] = {&GPIO4, &GPIO4, &GPIO4, &GPIO5, &GPIO8};
const uint8_t LED_PORT[LED_COUNT] = {BIT5, BIT6, BIT7, BIT7, BIT0};

const GPIO_TypeDef* BTN_GPIO[BTN_COUNT] = {&GPIO4, &GPIO4, &GPIO4, &GPIO4, &GPIO4};
const uint8_t BTN_PORT[BTN_COUNT] = {BIT4, BIT3, BIT2, BIT1, BIT0};

void initButtonsAndLeds()
{
  int i;
  for(i=0;i<LED_COUNT;++i) //P4.5不能用
    {
	  *LED_GPIO[i]->PxDIR |= LED_PORT[i]; //设置各LED灯所在端口为输出方向
	  *LED_GPIO[i]->PxOUT &= ~LED_PORT[i];
    }

  for(i=0;i<BTN_COUNT;++i)
  {
    *BTN_GPIO[i]->PxREN |= BTN_PORT[i]; //使能上下拉电阻
    *BTN_GPIO[i]->PxOUT |= BTN_PORT[i]; //设为上拉状态
  }
}

void led_SetLed(int LEDn, int on)
{
  LEDn = 5 - LEDn;
  if(LEDn < 0 || LEDn >=LED_COUNT)
    return;
  if(on)
    *LED_GPIO[LEDn]->PxOUT |= LED_PORT[LEDn];
  else
    *LED_GPIO[LEDn]->PxOUT &=~LED_PORT[LEDn];
}

int btn_Pressed(int Sn)
{
#ifdef REF_CLOCK
  static int32_t last = 0;
  if(REF_CLOCK - last < BUTTON_DEADZONE_PERIOD)
    return 0; //死区范围内
#endif

  Sn -= 3; //S3~S7 -> 0~4
  if(~(*BTN_GPIO[Sn]->PxIN) & BTN_PORT[Sn])
  {
#ifdef REF_CLOCK
    last = REF_CLOCK;
#endif
    return 1;
  }
  else
  {
    return 0;
  }
}

int btn_GetBtnInput(int block)
{
  while(1)
  {
    int i;
    for(i=3;i<=7;++i)
    {
      if(btn_Pressed(i))
        return i;
    }
    if(!block)
      return 0;
  }
}
