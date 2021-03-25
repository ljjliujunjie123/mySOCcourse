/*
 * main.c
 */
#include <msp430f6638.h>

void step(int p)
{
  switch(p)
  {
  case 0:
    {
      P7OUT &=~ BIT4;
      P1OUT &=~ BIT2;
      P2OUT &=~ BIT2;
      P1OUT &=~ BIT4;
      P2OUT &=~ BIT3;
      break;
    }
  case 1:
    {
      P7OUT |= BIT4;
      P1OUT |= BIT2;
      P2OUT &=~ BIT2;
      P1OUT &=~ BIT4;
      P2OUT &=~ BIT3;
      break;
    }
  case 2:
    {
      P7OUT |= BIT4;
      P1OUT &=~ BIT2;
      P2OUT |= BIT2;
      P1OUT &=~ BIT4;
      P2OUT &=~ BIT3;
      break;
    }
  case 3:
    {
      P7OUT |= BIT4;
      P1OUT &=~ BIT2;
      P2OUT &=~ BIT2;
      P1OUT |= BIT4;
      P2OUT &=~ BIT3;
      break;
    }
  case 4:
    {
      P7OUT |= BIT4;
      P1OUT &=~ BIT2;
      P2OUT &=~ BIT2;
      P1OUT &=~ BIT4;
      P2OUT |= BIT3;
      break;
    }
  default:
      break;
  }
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	P1DIR |= BIT2 + BIT4;
	P2DIR |= BIT2 + BIT3;
	P7DIR |= BIT4;
	P7OUT |= BIT4;
	int a=0;
	int b[8]={1,3,2,4,1,4,2,3};		//b[0]~b[3]正转，b[4]~b[7]翻转，
	
	while(1)
	{
	      step(b[a++]);				//正传
	      if (a>3)
	        a=0;
	      __delay_cycles(160000);
	}
}

