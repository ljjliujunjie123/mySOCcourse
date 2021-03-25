#include <msp430.h>
#include <stdint.h>
#include "dr_lcdseg.h"
#include "fw_public.h"
#define captouch_max 250
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

const GPIO_TypeDef* LED_GPIO[5] = {&GPIO4, &GPIO4, &GPIO4, &GPIO5, &GPIO8};
const uint8_t LED_PORT[5] = {BIT5, BIT6, BIT7, BIT7, BIT0};
//��������P6.1~P6.5
#define ALL_PORT (BIT1+BIT2+BIT3+BIT4+BIT5)

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

void initCapTouch()
{
  //Vref�ӵ�+��
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

  CBCTL0 = CBIMEN + (i << 8); //�ⲿ�źżӵ�����
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

int main( void )
{
  int i;
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  _DINT();
  initClock();
  initCapTouch();
  for(i=0;i<5;++i)
    *LED_GPIO[i]->PxDIR |= LED_PORT[i]; //���ø�LED�����ڶ˿�Ϊ�������
  _EINT();

  while(1)
  {
    uint32_t temp[5] ={0,0,0,0,0};

    for(i=5;i>=1;i--)
    {

      temp[i-1]= CapTouch_ReadChannel(i);
      if(temp[i-1]>captouch_max)
        *LED_GPIO[i-1]->PxOUT ^= LED_PORT[i-1]; //��ת��Ӧ��LED

   }
    __delay_cycles(8000000);
  }
  return 0;
}
