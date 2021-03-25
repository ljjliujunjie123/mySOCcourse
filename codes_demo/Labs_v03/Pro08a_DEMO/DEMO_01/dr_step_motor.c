#include <msp430.h>
#include <stdint.h>
#include "dr_step_motor.h"
#include "fw_public.h"

//有以下define时使用1.9版原理图的IO设置，没有时为最新IO设置(1.93版)
//#define IO_CONFIG_V1.9

#define CARRIER_FREQ     10000
#define TIMER_FULL_SCALE (SMCLK_FREQ / CARRIER_FREQ / 2) //三角载波

#ifdef IO_CONFIG_V19
  #define DRIVE_ENABLE_PORT_DIR   P2DIR |= BIT5  //设定DRV8833芯片使能端口为输出方向
  #define MOTOR_DRIVE_ENABLE      P2OUT |= BIT5  //DRV8833芯片使能
  #define MOTOR_DRIVE_DISABLE     P2OUT &= ~BIT5 //DRV8833芯片关断
#else
  #define DRIVE_ENABLE_PORT_DIR   P7DIR |= BIT4  //设定DRV8833芯片使能端口为输出方向
  #define MOTOR_DRIVE_ENABLE      P7OUT |= BIT4  //DRV8833芯片使能
  #define MOTOR_DRIVE_DISABLE     P7OUT &= ~BIT4 //DRV8833芯片关断
#endif

//A相绕组PWM相关寄存器，A相PWM在正极
#define PWM_APOS_CCR TA0CCR1
#define PWM_APOS_CTL TA0CCTL1
//A相负极IO
#define ANEG_SET        P2OUT |= BIT2
#define ANEG_RST        P2OUT &= ~BIT2
//B相正极IO
#define BPOS_SET        P2OUT |= BIT3
#define BPOS_RST        P2OUT &= ~BIT3
//B相绕组PWM相关寄存器，B相PWM在负极
#define PWM_BNEG_CCR TA0CCR3
#define PWM_BNEG_CTL TA0CCTL3

uint16_t step_per_tick = 0; //15位定点小数
uint16_t cur_step = 0; //15位定点小数

int32_t steps_to_move = 0;

void initStepMotor()
{
  //驱动软件初始化
  steps_to_move = 0;
  step_per_tick = 0;
  cur_step = 0;


  //驱动IO初始化
  MOTOR_DRIVE_DISABLE; //先置低
  DRIVE_ENABLE_PORT_DIR; //使能端口输出方向
  
  //定时器
  TA0CTL = TASSEL__SMCLK + MC__UPDOWN; //三角载波
  TA0CCR0 = TIMER_FULL_SCALE;
  TA0CCTL0 = CCIE; //10kHz中断

#ifdef IO_CONFIG_V19
  //整步与细分通用
  P1OUT &= ~(BIT1 + BIT6 + BIT7 + BIT4);
  P1DIR |= BIT1 + BIT6 + BIT7 + BIT4;
  //细分模式IO
  P1SEL |= BIT6 + BIT7 + BIT4; //选择定时器功能
  //细分模式PWM
  TA0CCR1 = 0; //A2
  TA0CCR2 = 0; //B1
  TA0CCR3 = 0; //B2
#else
  //电机驱动IO设定
  P1SEL |= BIT2 + BIT4; //选择定时器功能
  P1DIR |= BIT2 + BIT4; //输出方向
  P2OUT &= ~(BIT2 + BIT3);  //输出低电平
  P2DIR |= BIT2 + BIT3; //输出方向
  P2SEL &= ~(BIT2 + BIT3);
  //细分模式PWM
  PWM_APOS_CCR = 0; //AOUT1
  PWM_APOS_CTL = OUTMOD_5; //reset，输出低电平
  PWM_BNEG_CCR = 0; //BOUT2
  PWM_BNEG_CTL = OUTMOD_5; //reset，输出低电平
#endif
  
  initrpm();
  //进入初始状态
  StepMotor_MoveOneStep(1);
}

//细分模式输出表
#define STEP_LIST_SIZE 72 //机械角度1°一步，电角度5°一步
extern const int16_t STEP_LIST[STEP_LIST_SIZE * 2];
#include "dr_step_motor_list.h"

static inline int16_t StepMotor_CalTimerCnt(int16_t val)
{ //从电压标幺值计算得定时器设定值
  int32_t temp = val;
  temp *= TIMER_FULL_SCALE;
  temp >>= 15;
  return temp;
}

#ifdef IO_CONFIG_V19

static inline void StepMotor_SetVa(int16_t val)
{ //val为电压标幺值，15位定点小数
  val = StepMotor_CalTimerCnt(val);
  if(val >= 0)
  {
    P1OUT |= BIT1; //正向
    TA0CCTL1 = OUTMOD_6; //toggle/set模式
    TA0CCR1 = val;
  }
  else
  {
    P1OUT &= ~BIT1; //反向
    TA0CCTL1 = OUTMOD_2; //toggle/reset模式
    TA0CCR1 = (TIMER_FULL_SCALE - val) >> 1; //由于全零输出高阻态，等效于加反压，故占空比50%对应实际0V
  }
}

static inline void StepMotor_SetVb(int16_t val)
{ //val为电压标幺值，15位定点小数
  val = StepMotor_CalTimerCnt(val);
  /*TA0CCTL2 = OUTMOD_6; //toggle/set模式
  TA0CCR2 = (TIMER_FULL_SCALE - val) >> 1; //B1
  TA0CCTL3 = OUTMOD_6; //toggle/set模式
  TA0CCR3 = (TIMER_FULL_SCALE + val) >> 1; //B2*/
  if(val >= 0)
  {
    TA0CCTL2 = OUTMOD_1; //set模式
    TA0CCR2 = val; //B1
    TA0CCTL3 = OUTMOD_6; //toggle/set模式
    TA0CCR3 = val; //B2
  }
  else
  {
    TA0CCTL2 = OUTMOD_6; //toggle/set模式
    TA0CCR2 = -val; //B1
    TA0CCTL3 = OUTMOD_1; //set模式
    TA0CCR3 = -val; //B2
  }
}

#else

static inline void StepMotor_SetVa(int16_t val) //A相PWM在正极
{ //val为电压标幺值，15位定点小数
  val = StepMotor_CalTimerCnt(val);
  if(val >= 0) //A相输出正电压
  {
    PWM_APOS_CCR = (TIMER_FULL_SCALE + val) >> 1; //由于全零输出高阻态，等效于加反压，故占空比50%对应实际0V
    PWM_APOS_CTL = OUTMOD_2; //toggle/reset模式，val越大高电平越长，等效电压越高
    ANEG_RST; //负极输出低电平
  }
  else //A相输出负电压
  {
    PWM_APOS_CCR = -val; //取负得绝对值
    PWM_APOS_CTL = OUTMOD_6; //toggle/set模式，val越小(绝对值越大)则低电平时间越长，等效电压越低
    ANEG_SET; //负极输出高电平
  }
}

static inline void StepMotor_SetVb(int16_t val) //B相PWM在负极
{ //val为电压标幺值，15位定点小数
  val = StepMotor_CalTimerCnt(val);
  if(val >= 0) //B相输出正电压
  {
    BPOS_SET; //正极输出高电平
    PWM_BNEG_CCR = val;
    PWM_BNEG_CTL = OUTMOD_6; //toggle/set模式，val越大则低电平时间越长，等效电压越高
  }
  else //B相输出负电压
  {
    BPOS_RST; //正极输出低电平
    PWM_BNEG_CCR = (TIMER_FULL_SCALE - val) >> 1; //由于全零输出高阻态，等效于加反压，故占空比50%对应实际0V
    PWM_BNEG_CTL = OUTMOD_2; //toggle/reset模式，val越小(绝对值越大)则高电平时间越长，等效电压越低
  }
}

#endif //IO_CONFIG_V1.9

void StepMotor_MoveOneStep(int dir)
{
  static int step = -1;
  
  if(dir>=0)
    step++;
  else 
    step--;
  
  if(step < 0)
    step += STEP_LIST_SIZE;
  if(step >= STEP_LIST_SIZE)
    step -= STEP_LIST_SIZE;
  
  StepMotor_SetVa(STEP_LIST[step * 2]);
  StepMotor_SetVb(STEP_LIST[step * 2 + 1]);
}

void StepMotor_SetEnable(int enable)
{
  if(enable)
    MOTOR_DRIVE_ENABLE;
  else
    MOTOR_DRIVE_DISABLE;
}

void StepMotor_SetRPM(uint16_t rpm)
{
  uint16_t gie;
  _ECRIT(gie);
  step_per_tick = ((uint32_t)rpm) * (360L * 32768L / 60L) / CARRIER_FREQ; 
  _LCRIT(gie);
}

void StepMotor_AddStep(int32_t steps)
{
  uint16_t gie;
  _ECRIT(gie);
  steps_to_move += steps;
  _LCRIT(gie);
}

void StepMotor_ClearStep()
{
  uint16_t gie;
  _ECRIT(gie);
  steps_to_move = 0;
  _LCRIT(gie);
}

void stepMotorInterrupt() //由直流电机程序中的中断来调用
{
  cur_step += step_per_tick;
  if(cur_step >= 0x8000)
  {
    cur_step -= 0x8000;
    if(steps_to_move > 0)
    {
      steps_to_move--;
      StepMotor_MoveOneStep(1);
    }
    else if(steps_to_move < 0)
    {
      steps_to_move++;
      StepMotor_MoveOneStep(-1);
    }
  }
}
