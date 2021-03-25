#include <msp430.h>
#include <stdint.h>
#include "dr_step_motor.h"
#include "fw_public.h"

//������defineʱʹ��1.9��ԭ��ͼ��IO���ã�û��ʱΪ����IO����(1.93��)
//#define IO_CONFIG_V1.9

#define CARRIER_FREQ     10000
#define TIMER_FULL_SCALE (SMCLK_FREQ / CARRIER_FREQ / 2) //�����ز�

#ifdef IO_CONFIG_V19
  #define DRIVE_ENABLE_PORT_DIR   P2DIR |= BIT5  //�趨DRV8833оƬʹ�ܶ˿�Ϊ�������
  #define MOTOR_DRIVE_ENABLE      P2OUT |= BIT5  //DRV8833оƬʹ��
  #define MOTOR_DRIVE_DISABLE     P2OUT &= ~BIT5 //DRV8833оƬ�ض�
#else
  #define DRIVE_ENABLE_PORT_DIR   P7DIR |= BIT4  //�趨DRV8833оƬʹ�ܶ˿�Ϊ�������
  #define MOTOR_DRIVE_ENABLE      P7OUT |= BIT4  //DRV8833оƬʹ��
  #define MOTOR_DRIVE_DISABLE     P7OUT &= ~BIT4 //DRV8833оƬ�ض�
#endif

//A������PWM��ؼĴ�����A��PWM������
#define PWM_APOS_CCR TA0CCR1
#define PWM_APOS_CTL TA0CCTL1
//A�ฺ��IO
#define ANEG_SET        P2OUT |= BIT2
#define ANEG_RST        P2OUT &= ~BIT2
//B������IO
#define BPOS_SET        P2OUT |= BIT3
#define BPOS_RST        P2OUT &= ~BIT3
//B������PWM��ؼĴ�����B��PWM�ڸ���
#define PWM_BNEG_CCR TA0CCR3
#define PWM_BNEG_CTL TA0CCTL3

uint16_t step_per_tick = 0; //15λ����С��
uint16_t cur_step = 0; //15λ����С��

int32_t steps_to_move = 0;

void initStepMotor()
{
  //���������ʼ��
  steps_to_move = 0;
  step_per_tick = 0;
  cur_step = 0;


  //����IO��ʼ��
  MOTOR_DRIVE_DISABLE; //���õ�
  DRIVE_ENABLE_PORT_DIR; //ʹ�ܶ˿��������
  
  //��ʱ��
  TA0CTL = TASSEL__SMCLK + MC__UPDOWN; //�����ز�
  TA0CCR0 = TIMER_FULL_SCALE;
  TA0CCTL0 = CCIE; //10kHz�ж�

#ifdef IO_CONFIG_V19
  //������ϸ��ͨ��
  P1OUT &= ~(BIT1 + BIT6 + BIT7 + BIT4);
  P1DIR |= BIT1 + BIT6 + BIT7 + BIT4;
  //ϸ��ģʽIO
  P1SEL |= BIT6 + BIT7 + BIT4; //ѡ��ʱ������
  //ϸ��ģʽPWM
  TA0CCR1 = 0; //A2
  TA0CCR2 = 0; //B1
  TA0CCR3 = 0; //B2
#else
  //�������IO�趨
  P1SEL |= BIT2 + BIT4; //ѡ��ʱ������
  P1DIR |= BIT2 + BIT4; //�������
  P2OUT &= ~(BIT2 + BIT3);  //����͵�ƽ
  P2DIR |= BIT2 + BIT3; //�������
  P2SEL &= ~(BIT2 + BIT3);
  //ϸ��ģʽPWM
  PWM_APOS_CCR = 0; //AOUT1
  PWM_APOS_CTL = OUTMOD_5; //reset������͵�ƽ
  PWM_BNEG_CCR = 0; //BOUT2
  PWM_BNEG_CTL = OUTMOD_5; //reset������͵�ƽ
#endif
  
  initrpm();
  //�����ʼ״̬
  StepMotor_MoveOneStep(1);
}

//ϸ��ģʽ�����
#define STEP_LIST_SIZE 72 //��е�Ƕ�1��һ������Ƕ�5��һ��
extern const int16_t STEP_LIST[STEP_LIST_SIZE * 2];
#include "dr_step_motor_list.h"

static inline int16_t StepMotor_CalTimerCnt(int16_t val)
{ //�ӵ�ѹ����ֵ����ö�ʱ���趨ֵ
  int32_t temp = val;
  temp *= TIMER_FULL_SCALE;
  temp >>= 15;
  return temp;
}

#ifdef IO_CONFIG_V19

static inline void StepMotor_SetVa(int16_t val)
{ //valΪ��ѹ����ֵ��15λ����С��
  val = StepMotor_CalTimerCnt(val);
  if(val >= 0)
  {
    P1OUT |= BIT1; //����
    TA0CCTL1 = OUTMOD_6; //toggle/setģʽ
    TA0CCR1 = val;
  }
  else
  {
    P1OUT &= ~BIT1; //����
    TA0CCTL1 = OUTMOD_2; //toggle/resetģʽ
    TA0CCR1 = (TIMER_FULL_SCALE - val) >> 1; //����ȫ���������̬����Ч�ڼӷ�ѹ����ռ�ձ�50%��Ӧʵ��0V
  }
}

static inline void StepMotor_SetVb(int16_t val)
{ //valΪ��ѹ����ֵ��15λ����С��
  val = StepMotor_CalTimerCnt(val);
  /*TA0CCTL2 = OUTMOD_6; //toggle/setģʽ
  TA0CCR2 = (TIMER_FULL_SCALE - val) >> 1; //B1
  TA0CCTL3 = OUTMOD_6; //toggle/setģʽ
  TA0CCR3 = (TIMER_FULL_SCALE + val) >> 1; //B2*/
  if(val >= 0)
  {
    TA0CCTL2 = OUTMOD_1; //setģʽ
    TA0CCR2 = val; //B1
    TA0CCTL3 = OUTMOD_6; //toggle/setģʽ
    TA0CCR3 = val; //B2
  }
  else
  {
    TA0CCTL2 = OUTMOD_6; //toggle/setģʽ
    TA0CCR2 = -val; //B1
    TA0CCTL3 = OUTMOD_1; //setģʽ
    TA0CCR3 = -val; //B2
  }
}

#else

static inline void StepMotor_SetVa(int16_t val) //A��PWM������
{ //valΪ��ѹ����ֵ��15λ����С��
  val = StepMotor_CalTimerCnt(val);
  if(val >= 0) //A���������ѹ
  {
    PWM_APOS_CCR = (TIMER_FULL_SCALE + val) >> 1; //����ȫ���������̬����Ч�ڼӷ�ѹ����ռ�ձ�50%��Ӧʵ��0V
    PWM_APOS_CTL = OUTMOD_2; //toggle/resetģʽ��valԽ��ߵ�ƽԽ������Ч��ѹԽ��
    ANEG_RST; //��������͵�ƽ
  }
  else //A���������ѹ
  {
    PWM_APOS_CCR = -val; //ȡ���þ���ֵ
    PWM_APOS_CTL = OUTMOD_6; //toggle/setģʽ��valԽС(����ֵԽ��)��͵�ƽʱ��Խ������Ч��ѹԽ��
    ANEG_SET; //��������ߵ�ƽ
  }
}

static inline void StepMotor_SetVb(int16_t val) //B��PWM�ڸ���
{ //valΪ��ѹ����ֵ��15λ����С��
  val = StepMotor_CalTimerCnt(val);
  if(val >= 0) //B���������ѹ
  {
    BPOS_SET; //��������ߵ�ƽ
    PWM_BNEG_CCR = val;
    PWM_BNEG_CTL = OUTMOD_6; //toggle/setģʽ��valԽ����͵�ƽʱ��Խ������Ч��ѹԽ��
  }
  else //B���������ѹ
  {
    BPOS_RST; //��������͵�ƽ
    PWM_BNEG_CCR = (TIMER_FULL_SCALE - val) >> 1; //����ȫ���������̬����Ч�ڼӷ�ѹ����ռ�ձ�50%��Ӧʵ��0V
    PWM_BNEG_CTL = OUTMOD_2; //toggle/resetģʽ��valԽС(����ֵԽ��)��ߵ�ƽʱ��Խ������Ч��ѹԽ��
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

void stepMotorInterrupt() //��ֱ����������е��ж�������
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
