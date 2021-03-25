#include <msp430.h>
#include <stdint.h>
#include <string.h>
#include "dr_dc_motor.h"

#define DRIVE_ENABLE_PORT_DIR   P1DIR |= BIT0  //�趨DRV8833оƬʹ�ܶ˿�Ϊ�������
#define MOTOR_DRIVE_ENABLE      P1OUT |= BIT0  //DRV8833оƬʹ��
#define MOTOR_DRIVE_DISABLE     P1OUT &= ~BIT0 //DRV8833оƬ�ض�

#define PWM_POS_CCR TA0CCR1   //������Ӧ��CCR�Ĵ���
#define PWM_POS_CTL TA0CCTL1  //������Ӧ��CCTL�Ĵ���
#define PWM_NEG_CCR TA0CCR2   //������Ӧ��CCR�Ĵ���
#define PWM_NEG_CTL TA0CCTL2  //������Ӧ��CCTL�Ĵ���

#define SPEED_MEAS_INTVEC   PORT2_VECTOR   //ת�ٲ����ж�����
#define TOGGLE_MEAS_EDGE    P2IES ^= BIT0  //��ת�����������
#define TOGGLE_MEAS_EDGES   P2IES ^= BIT1  //��ת�����������
#define CLEAR_MEAS_INT_FLAG P2IFG = 0      //��������жϱ�־

#define MCLK_FREQ  20000000
#define SMCLK_FREQ 20000000

#define PWM_FREQ     10000
#define PWM_COUNT (SMCLK_FREQ / PWM_FREQ - 1)

uint32_t motor_refClock = 0; //��ת���òο�ʱ�ӣ�Ŀǰ�趨��0.1ms��һ��(��PWM�ز�Ƶ����ͬ)

//ת�ٲ���ʱ�Զ��ٴ�����������ƽ��
//����Ϊ2��n�η�
#define RPM_AVERAGE_COUNT 64
//RPM_AVERAGE_COUNT��2�Ķ��ٴη�
#define RPG_AVG_CNT_LOG    6
//��������ʱ��ֵ��2500��Ӧ250ms/60rpm
#define PUL_INT_OT_THD  2500L

struct RPM_MEAS_STRUCT //����ת�ٲ���״̬���õĽṹ��
{
  int forward; //�Ƿ���������ת(Ŀǰʵ�����������ѹ������ȷ����)
  uint32_t lastClkCnt; //��һ����Ч������ֵ�ʱ��

  int lastInte; //��һ��������ȣ����ڶ��������ص��ܼ�����

  int intePtr; //��ɵ�һ�����������ݼ�¼��λ��
  int32_t intervals[RPM_AVERAGE_COUNT]; //���������ݼ�¼
  int32_t inteSum[RPG_AVG_CNT_LOG + 1]; //���������ݼ�¼֮��
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
//  P2IE |= BIT1; //P2.0�ж�
  initrpm();
}

void initrpm()
{
	  //ת�ٲ�����������
	  memset(&rpm_meas, 0, sizeof(rpm_meas));
	  rpm_meas.forward = 1;
	  rpm_meas.lastInte = -1; //���û�������¼
	  int i;
	  for(i=0;i<RPM_AVERAGE_COUNT;++i)
	    rpm_meas.intervals[i] = PUL_INT_OT_THD + 1; //ȫ����Ϊ��ʱ
	  for(i=0;i<=RPG_AVG_CNT_LOG;++i)
	    rpm_meas.inteSum[i] = (PUL_INT_OT_THD + 1) << i; //ȫ����Ϊ��ʱ
}



#define ARR_OFFSET(base, offset, len) ((base + offset) & (len - 1))

static inline void updateRPMMeas()
{
  uint32_t curInte = motor_refClock - rpm_meas.lastClkCnt;
  rpm_meas.lastClkCnt = motor_refClock; //��¼��ǰʱ��

  if(curInte > PUL_INT_OT_THD)
    rpm_meas.lastInte = -1; //ת�ٹ�����Ч���������������ۼӣ�ֱ�Ӹ�����ͽ��
  else if(rpm_meas.lastInte == -1) //��û�е�һ��������صļ�¼
  {
    rpm_meas.lastInte = curInte; //��¼��һ��������ؼ��
    return; //��ʱ�޽�һ��������ֱ�ӷ���
  }
  else //�е�һ��������ؼ�¼
  {
    curInte += rpm_meas.lastInte; //������������ʱ��
    rpm_meas.lastInte = -1; //��ǵ�ǰû�������¼
  }

  //��curInte������ͽ��
  //ֱ��д�����������Ч��
  rpm_meas.inteSum[0] = curInte;
  rpm_meas.inteSum[1] += curInte - rpm_meas.intervals[ARR_OFFSET(rpm_meas.intePtr,  -2, RPM_AVERAGE_COUNT)];
  rpm_meas.inteSum[2] += curInte - rpm_meas.intervals[ARR_OFFSET(rpm_meas.intePtr,  -4, RPM_AVERAGE_COUNT)];
  rpm_meas.inteSum[3] += curInte - rpm_meas.intervals[ARR_OFFSET(rpm_meas.intePtr,  -8, RPM_AVERAGE_COUNT)];
  rpm_meas.inteSum[4] += curInte - rpm_meas.intervals[ARR_OFFSET(rpm_meas.intePtr, -16, RPM_AVERAGE_COUNT)];
  rpm_meas.inteSum[5] += curInte - rpm_meas.intervals[ARR_OFFSET(rpm_meas.intePtr, -32, RPM_AVERAGE_COUNT)];
  rpm_meas.inteSum[6] += curInte - rpm_meas.intervals[rpm_meas.intePtr];
  rpm_meas.intervals[rpm_meas.intePtr] = curInte; //������ɵ�����
  rpm_meas.intePtr = ARR_OFFSET(rpm_meas.intePtr, 1, RPM_AVERAGE_COUNT);
}

void stepMotorInterrupt();

#pragma vector=TIMER0_A0_VECTOR
__interrupt void softTimerInterrupt()
{
  motor_refClock++; //ȫ�ֲο�ʱ������

  if(motor_refClock - rpm_meas.lastClkCnt > PUL_INT_OT_THD) //���������ֱ�Ӱ�250ms��¼(60rpm)
  {
    updateRPMMeas();
  }

  stepMotorInterrupt();
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
    PWM_POS_CCR = -val;
    PWM_POS_CTL = OUTMOD_3; //set/reset��valԽ�󣬵͵�ƽʱ��Խ������Ч��ѹԽ��
    PWM_NEG_CCR = -val;
    PWM_NEG_CTL = OUTMOD_1; //set������ߵ�ƽ
    rpm_meas.forward = 0;
  }
}

#pragma vector=SPEED_MEAS_INTVEC
__interrupt void speedMeasureInterrupt() //ת�ٲ������ж�
{
  TOGGLE_MEAS_EDGE; //��ת���ط���
  TOGGLE_MEAS_EDGES;
  if(motor_refClock - rpm_meas.lastClkCnt >= 2) //ֻ�м������0.2ms����Ϊ��Ч(0.2ms���ٶ�Ӧת��9000rpm���ǲ����ܵ�)
  {
    updateRPMMeas();
  }
  CLEAR_MEAS_INT_FLAG; //����жϱ�־
}

int getRPM() //���ݵ�ǰ��ת�ٲ����������ת��ֵ����λΪRPM
{
  if(rpm_meas.inteSum[0] > PUL_INT_OT_THD) //250ms����Ӧ�ٶȵ���60rpm����Ϊ�����Ч
  {
    return 0;
  }

  int32_t val = rpm_meas.inteSum[0]; //�ο����������ѡ����ٴ�ƽ��
  //����ת�٣���λrpm
  //60s * ÿ��ʱ�Ӽ����(10k) / (������ƽ��ֵ / ÿ��תһȦ�����)
  //val = 60L * 1000L * 10L * (1L) / 4L / val;
  if(val > 428) //��ƽ����350rpm
    val = 60L * 1000L * 10L * (1L) / 4L / val;
  else if(val > 300) //2��ƽ����500rpm
    val = 60L * 1000L * 10L * (2L) / 4L / rpm_meas.inteSum[1];
  else if(val > 200) //4��ƽ����750rpm
    val = 60L * 1000L * 10L * (4L) / 4L / rpm_meas.inteSum[2];
  else if(val > 150) //8��ƽ����1000rpm
    val = 60L * 1000L * 10L * (8L) / 4L / rpm_meas.inteSum[3];
  else if(val > 100) //16��ƽ����1500rpm
    val = 60L * 1000L * 10L * (16L) / 4L / rpm_meas.inteSum[4];
  else if(val > 71) //32��ƽ����2100rpm
    val = 60L * 1000L * 10L * (32L) / 4L / rpm_meas.inteSum[5];
  else //64��ƽ��
    val = 60L * 1000L * 10L * (64L) / 4L / rpm_meas.inteSum[6];

  if(rpm_meas.forward)
    return val;
  else
    return -val;
}
