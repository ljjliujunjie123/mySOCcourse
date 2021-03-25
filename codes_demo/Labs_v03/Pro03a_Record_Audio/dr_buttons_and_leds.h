#ifndef __DR_BUTTONS_AND_LEDS_H_
#define __DR_BUTTONS_AND_LEDS_H_

//按键连续触发计时参考时钟，应当为int32_t
#define REF_CLOCK refClock
//在检测到一次按键按下后多长时间内不再次触发
#define BUTTON_DEADZONE_PERIOD 200

void initButtonsAndLeds();

//LED1 ~ LED5
void led_SetLed(int LEDn, int on);

//S3 ~ S7
int btn_Pressed(int Sn);

//block模式下会等待到用户输入才返回，返回3~7
//非block模式下立刻返回，无输入时返回0
int btn_GetBtnInput(int block);

#endif //__DR_BUTTONS_AND_LEDS_H_
