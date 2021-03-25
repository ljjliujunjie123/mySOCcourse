#include <msp430.h>
#include <stdint.h>
#include <string.h>
#include "dr_dc_motor.h"
#include "clock_setting.h"

#define DRIVE_ENABLE_PORT_DIR   P1DIR |= BIT0  //�趨DRV8833оƬʹ�ܶ˿�Ϊ�������
#define MOTOR_DRIVE_ENABLE      P1OUT |= BIT0  //DRV8833оƬʹ��
#define MOTOR_DRIVE_DISABLE     P1OUT &= ~BIT0 //DRV8833оƬ�ض�

#define PWM_POS_CCR TA0CCR1   //������Ӧ��CCR�Ĵ���
#define PWM_POS_CTL TA0CCTL1  //������Ӧ��CCTL�Ĵ���
#define PWM_NEG_CCR TA0CCR2   //������Ӧ��CCR�Ĵ���
#define PWM_NEG_CTL TA0CCTL2  //������Ӧ��CCTL�Ĵ���

uint32_t refClock = 0; //��ת���òο�ʱ�ӣ�Ŀǰ�趨��0.1ms��һ��(��PWM�ز�Ƶ����ͬ)

#define RPM_AVERAGE_COUNT 128 //ת�ٲ���ʱ�Զ��ٴ�����������ƽ��

struct RPM_MEAS_STRUCT //����ת�ٲ���״̬���õĽṹ��
{
  int valid; //��ʼΪ-1��Ϊ1ʱ����Ч
  int forward; //�Ƿ���������ת(Ŀǰʵ�����������ѹ������ȷ����)
  uint32_t lastClkCnt; //��һ����Ч������ֵ�ʱ��
  uint32_t curClkCnt; //���һ����Ч������ֵ�ʱ��(���ڶԶ��������жϹ���)

  int intePtr; //��ɵ�һ�����������ݼ�¼��λ��
  int32_t intervals[RPM_AVERAGE_COUNT]; //���������ݼ�¼
  int32_t inteSum; //���������ݼ�¼֮��
} rpm_meas;

void initDCMotor()
{
  //IO�˿�����
  P1SEL |= BIT6 + BIT7; //P1.6��1.7��ΪPWM���
  P1DIR |= BIT6 + BIT7; //P1.6��1.7�������
  MOTOR_DRIVE_DISABLE; //����ʹ�����Ϊ�ͣ���ֹ�����ǰ����
  DRIVE_ENABLE_PORT_DIR; //�趨ʹ�ܶ˿�Ϊ�������
  //��ʱ������
  TA0CTL = TASSEL__SMCLK + MC__UP;
  TA0CCR0 = PWM_COUNT; //399
  TA0CCTL0 = CCIE; //100us�ж�
  PWM_POS_CCR = 0;
  PWM_POS_CTL = OUTMOD_5; //reset������͵�ƽ
  PWM_NEG_CCR = 0;
  PWM_NEG_CTL = OUTMOD_5; //reset������͵�ƽ
  //ת�ٲ�������
  P2IE |= BIT0; //P2.0�ж�
  memset(&rpm_meas, 0, sizeof(rpm_meas));
  rpm_meas.valid = -1;
  rpm_meas.forward = 1;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void softTimerInterrupt()
{
  refClock++; //ȫ�ֲο�ʱ������
}

void PWM_SetOutput(int val) // -399 ~ 399
{
  if(val == 0) //��ʱֱ�ӹض����
  {
      MOTOR_DRIVE_DISABLE;
  }
  else if(val > 0) //��ת����������ߵ�ƽ������PWM
  {
    if(val > PWM_COUNT)
      val = PWM_COUNT;

    MOTOR_DRIVE_ENABLE;
    PWM_POS_CCR = val;
    PWM_POS_CTL = OUTMOD_1; //set������ߵ�ƽ
    PWM_NEG_CCR = val;
    PWM_NEG_CTL = OUTMOD_3; //set/reset��valԽ�󣬵͵�ƽʱ��Խ������Ч��ѹԽ��
    rpm_meas.forward = 1;
  }
  else //��ת����������ߵ�ƽ������PWM
  {
    if(val < -PWM_COUNT)
      val = -PWM_COUNT;

    MOTOR_DRIVE_ENABLE;
    PWM_POS_CCR = val;
    PWM_POS_CTL = OUTMOD_3; //set/reset��valԽ�󣬵͵�ƽʱ��Խ������Ч��ѹԽ��
    PWM_NEG_CCR = val;
    PWM_NEG_CTL = OUTMOD_1; //set������ߵ�ƽ
    rpm_meas.forward = 0;
  }
}

#pragma vector=PORT2_VECTOR
__interrupt void p2Interrupt() //ת�ٲ������ж�
{
  P2IES ^= BIT0; //��ת���ط���
  if(refClock - rpm_meas.curClkCnt >= 2) //ֻ�м������0.2ms����Ϊ��Ч(0.2ms���ٶ�Ӧת��9000rpm���ǲ����ܵ�)
  {
    rpm_meas.lastClkCnt = rpm_meas.curClkCnt;
    rpm_meas.curClkCnt = refClock; //��¼��ǰʱ��
    if(rpm_meas.valid < 1) //���Ƿ��Ѿ���¼���㹻���ڼ����������
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
  P2IFG = 0; //����жϱ�־
}

int getRPM() //���ݵ�ǰ��ת�ٲ����������ת��ֵ����λΪRPM
{
  if(rpm_meas.valid < 1) //��δ��¼���㹻�����������
    return 0;

  if(refClock - rpm_meas.curClkCnt > 2500L) //250ms����Ӧ�ٶȵ���60rpm����Ϊ�����Ч
  {
    rpm_meas.valid = -1;
    return 0;
  }

  int32_t val = rpm_meas.inteSum; //�������������ֵ���ָı䣬�ȼ�¼ȡֵ
  //����ת�٣���λrpm
  //60s * ÿ��ʱ�Ӽ����(10k) / (������ƽ��ֵ / ÿ��תһȦ�����)
  val = 60L * 1000L * 10L * (RPM_AVERAGE_COUNT / 2) / 4L / val;
  if(rpm_meas.forward)
    return val;
  else
    return -val;
}
