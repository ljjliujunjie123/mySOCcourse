#ifndef __DR_DC_MOTOR_H_
#define __DR_DC_MOTOR_H_

//初始化直流电机模块，包括相关端口配置、定时器配置和转速测量配置
void initDCMotor();

//设定PWM输出值，设定值应当在-PWM_COUNT~+PWM_COUNT
void PWM_SetOutput(int val);

//根据当前的转速测量情况计算转速值，单位为RPM
int getRPM();

#endif
