#ifndef __DR_DC_MOTOR_H_
#define __DR_DC_MOTOR_H_

//��ʼ��ֱ�����ģ�飬������ض˿����á���ʱ�����ú�ת�ٲ�������
void initDCMotor();

//�趨PWM���ֵ���趨ֵӦ����-PWM_COUNT~+PWM_COUNT
void PWM_SetOutput(int val);

//���ݵ�ǰ��ת�ٲ����������ת��ֵ����λΪRPM
int getRPM();

#endif
