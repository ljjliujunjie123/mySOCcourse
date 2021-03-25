#include "include.h"

uint16_t adc0, adc0o;

//全局参考时钟
volatile int32_t refClock = 0;

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

  /* 初始化参考时钟 */
  TA2CTL = TASSEL__SMCLK + MC__UP + ID__1;
  TA2CCR0 = (SMCLK_FREQ / REFCLK_FREQ) - 1;
  TA2CCTL0 = CCIE;
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void refClockInterrupt()
{
  refClock++;

  app_kb_callback_scanner(); //矩阵键盘和LED矩阵的扫描器

  static int counter = 0, cnt2 = 0;
  counter++;
  if(counter >= 100 || cnt2 > 0)
  {
    counter = 0;
    if(cnt2 > 0)
      cnt2--;
    if(usbmsc_IdleTask()) //如果本周期有任务要处理，那么接下来20周期还尝试处理
      cnt2 = 20;
  }
}

void initADC()
{
  //内存变量初始化
  adc0 = 0;
  //IO设定
  P7SEL |= BIT7; //保证没有输出干扰模拟信号输入
  //TB0设定，用于ADC触发脉冲
  TB0CTL = TBSSEL__SMCLK + MC__UP; // SMCLK, up-mode
  TB0CCR0 = SMCLK_FREQ / 6400 - 1; // 4MHz -> 6.4kHz
  TB0CCTL0 = OUTMOD_4; //实际只有3.2kHz
  //ADC设定
  ADC12CTL1 = ADC12SHS_2 + ADC12SHP + ADC12CONSEQ_1; //TB0.0，脉冲采样，多通道单次
  ADC12MCTL0 = ADC12SREF_0 + ADC12INCH_15 + ADC12EOS; //3.3V电源参考，A15，通道末尾
  ADC12CTL0 = ADC12MSC + ADC12ON + ADC12SHT0_8 + ADC12ENC; //多重采样，ADC12ON，MEM0~MEM7使用256周期采样，ENC
  ADC12IE = BIT0; //开启中断(通道0转换完成后中断)
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  adc0o = ADC12MEM0; //原始结果，电机控制用
  adc0 = adc0o/585; //保存转换结果
//  adc0 = ADC12MEM0 * PWM_COUNT * 2 / 4095 - PWM_COUNT;
  ADC12CTL0 &= ~ADC12ENC; //重置ADC，准备下一次采样
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
    val = adc0o; //获取ADC转换结果
    val = val * PWM_COUNT * 2 / 4095 - PWM_COUNT; //将ADC转换结果从0~4095转换到-1999~1999
    LCDSEG_DisplayNumber(getRPM(), 0); //更新转速显示
    PWM_SetOutput(val); //修改PWM输出电压
    __delay_cycles(MCLK_FREQ / 10); //延时100ms
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
//	   temp = (cur_btn ^ last_btn) & last_btn; //找出刚向下跳变的按键
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


