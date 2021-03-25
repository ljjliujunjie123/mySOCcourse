#include <msp430.h>
#include <stdint.h>
#include <string.h>
#include "dr_dc_motor.h"

#define DRIVE_ENABLE_PORT_DIR   P1DIR |= BIT0  //设定DRV8833芯片使能端口为输出方向
#define MOTOR_DRIVE_ENABLE      P1OUT |= BIT0  //DRV8833芯片使能
#define MOTOR_DRIVE_DISABLE     P1OUT &= ~BIT0 //DRV8833芯片关断

#define PWM_POS_CCR TA0CCR1   //正极对应的CCR寄存器
#define PWM_POS_CTL TA0CCTL1  //正极对应的CCTL寄存器
#define PWM_NEG_CCR TA0CCR2   //负极对应的CCR寄存器
#define PWM_NEG_CTL TA0CCTL2  //负极对应的CCTL寄存器

#define SPEED_MEAS_INTVEC   PORT2_VECTOR   //转速测量中断向量
#define TOGGLE_MEAS_EDGE    P2IES ^= BIT0  //翻转测量边沿语句
#define TOGGLE_MEAS_EDGES   P2IES ^= BIT1  //翻转测量边沿语句
#define CLEAR_MEAS_INT_FLAG P2IFG = 0      //清除测量中断标志

#define MCLK_FREQ  20000000
#define SMCLK_FREQ 20000000

#define PWM_FREQ     10000
#define PWM_COUNT (SMCLK_FREQ / PWM_FREQ - 1)

uint32_t motor_refClock = 0; //测转速用参考时钟，目前设定下0.1ms走一次(与PWM载波频率相同)

//转速测量时对多少次脉冲间隔进行平均
//必须为2的n次方
#define RPM_AVERAGE_COUNT 64
//RPM_AVERAGE_COUNT是2的多少次方
#define RPG_AVG_CNT_LOG    6
//脉冲间隔超时阈值，2500对应250ms/60rpm
#define PUL_INT_OT_THD  2500L

struct RPM_MEAS_STRUCT //保存转速测量状态所用的结构体
{
  int forward; //是否是正向旋转(目前实际是由输出电压极性来确定的)
  uint32_t lastClkCnt; //上一次有效脉冲出现的时间

  int lastInte; //上一个间隔长度，用于对两个边沿的总间隔求和

  int intePtr; //最旧的一次脉冲间隔数据记录的位置
  int32_t intervals[RPM_AVERAGE_COUNT]; //脉冲间隔数据记录
  int32_t inteSum[RPG_AVG_CNT_LOG + 1]; //脉冲间隔数据记录之和
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
//  P2IE |= BIT1; //P2.0中断
  initrpm();
}

void initrpm()
{
	  //转速测量公共部分
	  memset(&rpm_meas, 0, sizeof(rpm_meas));
	  rpm_meas.forward = 1;
	  rpm_meas.lastInte = -1; //标记没有脉冲记录
	  int i;
	  for(i=0;i<RPM_AVERAGE_COUNT;++i)
	    rpm_meas.intervals[i] = PUL_INT_OT_THD + 1; //全部设为超时
	  for(i=0;i<=RPG_AVG_CNT_LOG;++i)
	    rpm_meas.inteSum[i] = (PUL_INT_OT_THD + 1) << i; //全部设为超时
}



#define ARR_OFFSET(base, offset, len) ((base + offset) & (len - 1))

static inline void updateRPMMeas()
{
  uint32_t curInte = motor_refClock - rpm_meas.lastClkCnt;
  rpm_meas.lastClkCnt = motor_refClock; //记录当前时间

  if(curInte > PUL_INT_OT_THD)
    rpm_meas.lastInte = -1; //转速过低无效，跳过两次脉冲累加，直接更新求和结果
  else if(rpm_meas.lastInte == -1) //还没有第一个脉冲边沿的记录
  {
    rpm_meas.lastInte = curInte; //记录第一个脉冲边沿间隔
    return; //此时无进一步操作，直接返回
  }
  else //有第一个脉冲边沿记录
  {
    curInte += rpm_meas.lastInte; //计算整个脉冲时长
    rpm_meas.lastInte = -1; //标记当前没有脉冲记录
  }

  //用curInte更新求和结果
  //直接写出以提高运行效率
  rpm_meas.inteSum[0] = curInte;
  rpm_meas.inteSum[1] += curInte - rpm_meas.intervals[ARR_OFFSET(rpm_meas.intePtr,  -2, RPM_AVERAGE_COUNT)];
  rpm_meas.inteSum[2] += curInte - rpm_meas.intervals[ARR_OFFSET(rpm_meas.intePtr,  -4, RPM_AVERAGE_COUNT)];
  rpm_meas.inteSum[3] += curInte - rpm_meas.intervals[ARR_OFFSET(rpm_meas.intePtr,  -8, RPM_AVERAGE_COUNT)];
  rpm_meas.inteSum[4] += curInte - rpm_meas.intervals[ARR_OFFSET(rpm_meas.intePtr, -16, RPM_AVERAGE_COUNT)];
  rpm_meas.inteSum[5] += curInte - rpm_meas.intervals[ARR_OFFSET(rpm_meas.intePtr, -32, RPM_AVERAGE_COUNT)];
  rpm_meas.inteSum[6] += curInte - rpm_meas.intervals[rpm_meas.intePtr];
  rpm_meas.intervals[rpm_meas.intePtr] = curInte; //更新最旧的数据
  rpm_meas.intePtr = ARR_OFFSET(rpm_meas.intePtr, 1, RPM_AVERAGE_COUNT);
}

void stepMotorInterrupt();

#pragma vector=TIMER0_A0_VECTOR
__interrupt void softTimerInterrupt()
{
  motor_refClock++; //全局参考时钟自增

  if(motor_refClock - rpm_meas.lastClkCnt > PUL_INT_OT_THD) //间隔过长，直接按250ms记录(60rpm)
  {
    updateRPMMeas();
  }

  stepMotorInterrupt();
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
    PWM_POS_CCR = -val;
    PWM_POS_CTL = OUTMOD_3; //set/reset，val越大，低电平时间越长，等效电压越低
    PWM_NEG_CCR = -val;
    PWM_NEG_CTL = OUTMOD_1; //set，输出高电平
    rpm_meas.forward = 0;
  }
}

#pragma vector=SPEED_MEAS_INTVEC
__interrupt void speedMeasureInterrupt() //转速测量用中断
{
  TOGGLE_MEAS_EDGE; //翻转边沿方向
  TOGGLE_MEAS_EDGES;
  if(motor_refClock - rpm_meas.lastClkCnt >= 2) //只有间隔超过0.2ms才认为有效(0.2ms至少对应转速9000rpm，是不可能的)
  {
    updateRPMMeas();
  }
  CLEAR_MEAS_INT_FLAG; //清除中断标志
}

int getRPM() //根据当前的转速测量情况计算转速值，单位为RPM
{
  if(rpm_meas.inteSum[0] > PUL_INT_OT_THD) //250ms，对应速度低于60rpm，认为结果无效
  {
    return 0;
  }

  int32_t val = rpm_meas.inteSum[0]; //参考间隔，决定选择多少次平均
  //计算转速，单位rpm
  //60s * 每秒时钟间隔数(10k) / (脉冲间隔平均值 / 每旋转一圈间隔数)
  //val = 60L * 1000L * 10L * (1L) / 4L / val;
  if(val > 428) //不平均，350rpm
    val = 60L * 1000L * 10L * (1L) / 4L / val;
  else if(val > 300) //2次平均，500rpm
    val = 60L * 1000L * 10L * (2L) / 4L / rpm_meas.inteSum[1];
  else if(val > 200) //4次平均，750rpm
    val = 60L * 1000L * 10L * (4L) / 4L / rpm_meas.inteSum[2];
  else if(val > 150) //8次平均，1000rpm
    val = 60L * 1000L * 10L * (8L) / 4L / rpm_meas.inteSum[3];
  else if(val > 100) //16次平均，1500rpm
    val = 60L * 1000L * 10L * (16L) / 4L / rpm_meas.inteSum[4];
  else if(val > 71) //32次平均，2100rpm
    val = 60L * 1000L * 10L * (32L) / 4L / rpm_meas.inteSum[5];
  else //64次平均
    val = 60L * 1000L * 10L * (64L) / 4L / rpm_meas.inteSum[6];

  if(rpm_meas.forward)
    return val;
  else
    return -val;
}
