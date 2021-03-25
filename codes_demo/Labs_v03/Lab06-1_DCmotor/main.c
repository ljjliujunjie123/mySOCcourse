/*
 * main.c
 */
#include <msp430f6638.h>

void DCmotor(int p)
{
  switch(p)
  {
  case 0:					//停转
    {
      P1OUT &=~ BIT0;
      P1OUT &=~ BIT6;
      P1OUT &=~ BIT7;
      break;
    }
  case 1:					//正转
    {
      P1OUT |= BIT0;
      P1OUT |= BIT6;
      P1OUT &=~ BIT7;
      break;
    }
  case 2:					//反转
    {
      P1OUT |= BIT0;
      P1OUT &=~ BIT6;
      P1OUT |= BIT7;
      break;
    }
  }
}

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
	P1DIR |= BIT0+BIT6+BIT7;
	DCmotor(1);
	__bis_SR_register(LPM3_bits);       // Enter LPM3
	__no_operation();                   // For debugger
}
