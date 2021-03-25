#ifndef __CLOCK_SETTING_H_
#define __CLOCK_SETTING_H_

#define XT2_FREQ   4000000

#define MCLK_FREQ 16000000
#define SMCLK_FREQ 4000000

#define PWM_FREQ     10000
#define PWM_COUNT (SMCLK_FREQ / PWM_FREQ - 1)

#endif
