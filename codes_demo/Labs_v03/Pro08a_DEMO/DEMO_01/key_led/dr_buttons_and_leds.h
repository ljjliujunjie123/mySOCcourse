#ifndef __DR_BUTTONS_AND_LEDS_H_
#define __DR_BUTTONS_AND_LEDS_H_

//��������������ʱ�ο�ʱ�ӣ�Ӧ��Ϊint32_t
#define REF_CLOCK refClock
//�ڼ�⵽һ�ΰ������º�೤ʱ���ڲ��ٴδ���
#define BUTTON_DEADZONE_PERIOD 200

void initButtonsAndLeds();

//LED1 ~ LED5
void led_SetLed(int LEDn, int on);

//S3 ~ S7
int btn_Pressed(int Sn);

//blockģʽ�»�ȴ����û�����ŷ��أ�����3~7
//��blockģʽ�����̷��أ�������ʱ����0
int btn_GetBtnInput(int block);

#endif //__DR_BUTTONS_AND_LEDS_H_
