#include "include.h"

uint16_t adc0, adc0o;

//ȫ�ֲο�ʱ��
volatile int32_t refClock = 0;

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

  /* ��ʼ���ο�ʱ�� */
  TA2CTL = TASSEL__SMCLK + MC__UP + ID__1;
  TA2CCR0 = (SMCLK_FREQ / REFCLK_FREQ) - 1;
  TA2CCTL0 = CCIE;
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void refClockInterrupt()
{
  refClock++;

  app_kb_callback_scanner(); //������̺�LED�����ɨ����

  static int counter = 0, cnt2 = 0;
  counter++;
  if(counter >= 100 || cnt2 > 0)
  {
    counter = 0;
    if(cnt2 > 0)
      cnt2--;
    if(usbmsc_IdleTask()) //���������������Ҫ������ô������20���ڻ����Դ���
      cnt2 = 20;
  }
}

void initADC()
{
  //�ڴ������ʼ��
  adc0 = 0;
  //IO�趨
  P7SEL |= BIT7; //��֤û���������ģ���ź�����
  //TB0�趨������ADC��������
  TB0CTL = TBSSEL__SMCLK + MC__UP; // SMCLK, up-mode
  TB0CCR0 = SMCLK_FREQ / 6400 - 1; // 4MHz -> 6.4kHz
  TB0CCTL0 = OUTMOD_4; //ʵ��ֻ��3.2kHz
  //ADC�趨
  ADC12CTL1 = ADC12SHS_2 + ADC12SHP + ADC12CONSEQ_1; //TB0.0�������������ͨ������
  ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_15 + ADC12EOS; //3.3V��Դ�ο���A15��ͨ��ĩβ
  ADC12CTL0 = ADC12MSC + ADC12ON + ADC12SHT0_8 + ADC12ENC; //���ز�����ADC12ON��MEM0~MEM7ʹ��256���ڲ�����ENC
  ADC12IE = BIT0; //�����ж�(ͨ��0ת����ɺ��ж�)
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  adc0o = ADC12MEM0; //ԭʼ��������������
  adc0 = adc0o/585; //����ת�����
//  adc0 = ADC12MEM0 * PWM_COUNT * 2 / 4095 - PWM_COUNT;
  ADC12CTL0 &= ~ADC12ENC; //����ADC��׼����һ�β���
  ADC12CTL0 |= ADC12ENC;
}

#define PWM_COUNT 1999
void app_DCMotor()
{
  initDCMotor();
  P2IE |= BIT0;

  etft_AreaSet(0,0,319,239,0);

  etft_DisplayString("DC Motor is Working...",40,60,65504,0);
  etft_DisplayString("Use R13 to Adjust Speed",40,100,65504,0);
  etft_DisplayString("Current Speed(RPM) Display at LCD",40,120,65504,0);
  etft_DisplayString("Press any key to exit...",40,140,65504,0);

  while(1)
  {
    int32_t val;
    val = adc0o; //��ȡADCת�����
    val = val * PWM_COUNT * 2 / 4095 - PWM_COUNT; //��ADCת�������0~4095ת����-1999~1999
    LCDSEG_DisplayNumber(getRPM(), 0); //����ת����ʾ
    PWM_SetOutput(val); //�޸�PWM�����ѹ
    __delay_cycles(MCLK_FREQ / 10); //��ʱ100ms
    if(btn_GetBtnInput(0))
    {
      PWM_SetOutput(0);
      int i;
      for(i=0;i<6;++i)
        LCDSEG_SetDigit(i, -1);
      	P2IE &=~ BIT0;
      return;
    }
  }
}

void app_StepMotor()
{
  int i;
  float steprpm;

  etft_AreaSet(0,0,319,239,0);
  etft_DisplayString("Step Motor is Working...",40,60,65504,0);
  etft_DisplayString("Use R13 to Adjust Speed",40,100,65504,0);
  etft_DisplayString("S4 : 1000 Round Clockwise",40,120,65504,0);
  etft_DisplayString("S6 : 1000 Round Anti-Clockwise",40,140,65504,0);
  etft_DisplayString("S5 : Stop and Exit",40,160,65504,0);

  initStepMotor();
  P2SEL &=~ BIT1;
  P2DIR &=~ BIT1;
  P2OUT &=~ BIT1;
  P2IE |= BIT1;
  StepMotor_SetEnable(1);
  while(1)
  {
    int16_t rpm = ((int32_t)adc0o) * 600L / 4096L;
//    LCDSEG_DisplayNumber(rpm, 0);
    StepMotor_SetRPM(rpm);

    int btn = btn_GetBtnInput(0);
    if(btn == 6) //S6
      {
    	StepMotor_AddStep(360000);
    	steprpm= -0.5;
      }
    else if(btn == 4) //S4
      {
    	StepMotor_AddStep(-360000);
    	steprpm= 0.5;
      }

    else if(btn == 5) //S5
      break;
    else if(btn == 7) //S7
      StepMotor_AddStep(-1);
    else if(btn == 3) //S3
      StepMotor_AddStep(1);
    LCDSEG_DisplayNumber(getRPM()*steprpm, 0);
    __delay_cycles(MCLK_FREQ / 10); //100ms
  }
  for(i=0;i<6;++i)
    LCDSEG_SetDigit(i, -1);
  StepMotor_ClearStep();
  StepMotor_SetEnable(0);
  P2IE &=~ BIT1;
}

int main( void )
{
  // Stop watchdog timer to prevent time out reset
  WDTCTL = WDTPW + WDTHOLD;

  _DINT();
  initClock();
  initADC();
  initTFT();
  initButtonsAndLeds();
  initAudioIO();
  initLcdSeg();
  _EINT();

  bmp_LogViewer();
  etft_AreaSet(0,0,319,239,0);
  int backcolor[7];
  initUSBMSC();
  //uint8_t last_btn = 0x1F, cur_btn, temp;
  while(1)
  {
//	  __delay_cycles(100000);
	  memset( backcolor, 0, sizeof(backcolor));
	  backcolor[adc0]=31;
	  etft_DisplayString("MENU",140,20,etft_Color(0, 255, 0),0);
	  etft_DisplayString("# BMP Image Display",40,60,65504,backcolor[0]);
	  etft_DisplayString("# Music Player",40,80,65504,backcolor[1]);
	  etft_DisplayString("# DC Motor",40,100,65504,backcolor[2]);
	  etft_DisplayString("# Step Motor",40,120,65504,backcolor[3]);
	  etft_DisplayString("# keyboard Module",40,140,65504,backcolor[4]);
	  etft_DisplayString("# Temp & Battery Monitor",40,160,65504,backcolor[5]);
	  etft_DisplayString("# Touch Keys",40,180,65504,backcolor[6]);
	  etft_DisplayString("R13: Select                    S5: Enter",1,220,65535,0);

//	  __delay_cycles(MCLK_FREQ*20);
//	   cur_btn = P4IN & 0x1F;
//	   temp = (cur_btn ^ last_btn) & last_btn; //�ҳ�����������İ���
//	   last_btn = cur_btn;
	   if(btn_GetBtnInput(0) == 5)
	   {
		   switch(adc0)
		   {
		   	   case 0:
		   	     bmp_BmpViewerMain();
		   		   break;
		   	   case 1:
		   	     app_StartWavPlayer();
		   		   break;
		   	   case 2:
		   	     app_DCMotor();
		   		   break;
		   	   case 3:
		   	     app_StepMotor();
		   		   break;
		   	   case 4:
		   	     app_keyboard();
		   		   break;
		   	   case 5:
		   	     BATTmain();
		   		   break;
		   	   case 6:
		   		 touchkeymain();
		   		   break;
		   	   default:
		   		   break;
		   }
		   etft_AreaSet(0,0,319,239,0);
	   }
  }
}


