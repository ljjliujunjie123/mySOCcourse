#ifndef __DR_STEP_MOTOR_H_
#define __DR_STEP_MOTOR_H_

#ifndef SMCLK_FREQ
  #define SMCLK_FREQ 4000000
#endif

void initStepMotor();

void StepMotor_SetEnable(int enable);
void StepMotor_MoveOneStep(int dir);

void StepMotor_SetRPM(uint16_t rpm);

void StepMotor_AddStep(int32_t steps);
void StepMotor_ClearStep();

#endif
