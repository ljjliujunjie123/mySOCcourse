#include <msp430.h>
#include <stdint.h>
#include <string.h>
#include "dr_dc_motor.h"
#include "clock_setting.h"

#define DRIVE_ENABLE_PORT_DIR   P1DIR |= BIT0  //设定DRV8833芯片使能端口为输出方向
#define MOTOR_DRIVE_ENABLE      P1OUT |= BIT0  //DRV8833芯片使能
#define MOTOR_DRIVE_DISABLE     P1OUT &= ~BIT0 //DRV8833芯片关断

#define PWM_POS_CCR TA0CCR1   //正极对应的CCR寄存器
#define PWM_POS_CTL TA0CCTL1  //正极对应的CCTL寄存器
#define PWM_NEG_CCR TA0CCR2   //负极对应的CCR寄存器
#define PWM_NEG_CTL TA0CCTL2  //负极对应的CCTL寄存器

uint32_t refClock = 0; //测转速用参考时钟，目前设定下0.1ms走一次(与PWM载波频率相同)

#define RPM_AVERAGE_COUNT 128 //转速测量时对多少次脉冲间隔进行平均

struct RPM_MEAS_STRUCT //保存转速测量状态所用的结构体
{
  int valid; //初始为-1，为1时才有效
  int forward; //是否是正向旋转(目前实际是由输出电压极性来确定的)
  uint32_t lastClkCnt; //上一次有效脉冲出现的时间
  uint32_t curClkCnt; //最近一次有效脉冲出现的时间(用于对抖动进行判断过滤)

  int intePtr; //最旧的一次脉冲间隔数据记录的位置
  int32_t intervals[RPM_AVERAGE_COUNT]; //脉冲间隔数据记录
  int32_t inteSum; //脉冲间隔数据记录之和
} rpm_meas;

void initDCMotor()
{
  //IO端口配置
  P1SEL |= BIT6 + BIT7; //P1.6、1.7作为PWM输出
  P1DIR |= BIT6 + BIT7; //P1.6、1.7输出方向
  MOTOR_DRIVE_DISABLE; //先置使能输出为低，防止电机提前启动
  DRIVE_ENABLE_PORT_DIR; //设定使能端口为输出方向
  //定时器配置
  TA0CTL = TASSEL__SMCLK + MC__UP;
  TA0CCR0 = PWM_COUNT; //399
  TA0CCTL0 = CCIE; //100us中断
  PWM_POS_CCR = 0;
  PWM_POS_CTL = OUTMOD_5; //reset，输出低电平
  PWM_NEG_CCR = 0;
  PWM_NEG_CTL = OUTMOD_5; //reset，输出低电平
  //转速测量配置
  P2IE |= BIT0; //P2.0中断
  memset(&rpm_meas, 0, sizeof(rpm_meas));
  rpm_meas.valid = -1;
  rpm_meas.forward = 1;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void softTimerInterrupt()
{
  refClock++; //全局参考时钟自增
}

void PWM_SetOutput(int val) // -399 ~ 399
{
  if(val == 0) //此时直接关断输出
  {
      MOTOR_DRIVE_DISABLE;
  }
  else if(val > 0) //正转，正极输出高电平，负极PWM
  {
    if(val > PWM_COUNT)
      val = PWM_COUNT;

    MOTOR_DRIVE_ENABLE;
    PWM_POS_CCR = val;
    PWM_POS_CTL = OUTMOD_1; //set，输出高电平
    PWM_NEG_CCR = val;
    PWM_NEG_CTL = OUTMOD_3; //set/reset，val越大，低电平时间越长，等效电压越高
    rpm_meas.forward = 1;
  }
  else //反转，负极输出高电平，正极PWM
  {
    if(val < -PWM_COUNT)
      val = -PWM_COUNT;

    MOTOR_DRIVE_ENABLE;
    PWM_POS_CCR = val;
    PWM_POS_CTL = OUTMOD_3; //set/reset，val越大，低电平时间越长，等效电压越低
    PWM_NEG_CCR = val;
    PWM_NEG_CTL = OUTMOD_1; //set，输出高电平
    rpm_meas.forward = 0;
  }
}

#pragma vector=PORT2_VECTOR
__interrupt void p2Interrupt() //转速测量用中断
{
  P2IES ^= BIT0; //翻转边沿方向
  if(refClock - rpm_meas.curClkCnt >= 2) //只有间隔超过0.2ms才认为有效(0.2ms至少对应转速9000rpm，是不可能的)
  {
    rpm_meas.lastClkCnt = rpm_meas.curClkCnt;
    rpm_meas.curClkCnt = refClock; //记录当前时间
    if(rpm_meas.valid < 1) //看是否已经记录到足够用于计算的脉冲数
      rpm_meas.valid++;
    else
    {
      rpm_meas.inteSum -= rpm_meas.intervals[rpm_meas.intePtr];
      rpm_meas.intervals[rpm_meas.intePtr] = refClock - rpm_meas.lastClkCnt;
      rpm_meas.inteSum += rpm_meas.intervals[rpm_meas.intePtr];
      rpm_meas.intePtr++;
      if(rpm_meas.intePtr >= RPM_AVERAGE_COUNT)
        rpm_meas.intePtr = 0;
    }
  }
  P2IFG = 0; //清除中断标志
}

int getRPM() //根据当前的转速测量情况计算转速值，单位为RPM
{
  if(rpm_meas.valid < 1) //还未记录到足够计算的脉冲数
    return 0;

  if(refClock - rpm_meas.curClkCnt > 2500L) //250ms，对应速度低于60rpm，认为结果无效
  {
    rpm_meas.valid = -1;
    return 0;
  }

  int32_t val = rpm_meas.inteSum; //避免运算过程中值出现改变，先记录取值
  //计算转速，单位rpm
  //60s * 每秒时钟间隔数(10k) / (脉冲间隔平均值 / 每旋转一圈间隔数)
  val = 60L * 1000L * 10L * (RPM_AVERAGE_COUNT / 2) / 4L / val;
  if(rpm_meas.forward)
    return val;
  else
    return -val;
}
