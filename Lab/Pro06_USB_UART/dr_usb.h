#ifndef __DR_USB_H_
#define __DR_USB_H_

/********************************
  USBģ��

!!!WinXP��Mac�޷�ʹ�ñ�ģ���Э�飬���Ӻ���ܲ�ʶ��

�ṩʹ��USB����ͨѶ�Ľӿڣ�ͬʱ�ṩCDC��HID����ͨ��(������Ϊchannel)
CDCͨ����PC�˽������⴮�ڣ���PC�˿���ֱ�Ӱ����ڷ���
HIDͨ����PC��ʶ��Ϊ����/��꣬��ͨ��TI�ı�׼�������ݴ��䣬��PC����Ҫʹ��TI�Ľӿڷ���
����ͨ����MSP430�ϵı�ģ����ʹ��ͬһ�ӿڽ��з��ʣ�ͨ��ͨ���Ž�������

��ʹ��USB_ReadChannel���ж�ȡʱ��Ҫע�⣬CDCģʽ����128B��HIDģʽ����64B���ڲ�������
���ڲ���������д��������δ������������ܵĺ������佫������

ʹ��USB_WriteChannel���з���ʱ������һ��������32K���ݣ����ڷ��Ͳ�������ǰ���ݻ�����Ӧ�������ݲ���

CDCģʽ���ͨѶ����788KB/s
HIDģʽ���ͨѶ����64KB/s
********************************/

#include <stdint.h>

//USB����״̬��ʶ����usblib430һ��
#define ST_USB_DISCONNECTED         0x80
#define ST_USB_CONNECTED_NO_ENUM    0x81
#define ST_ENUM_IN_PROGRESS         0x82
#define ST_ENUM_ACTIVE              0x83
#define ST_ENUM_SUSPENDED           0x84
#define ST_FAILED_ENUM              0x85
#define ST_ERROR                    0x86
#define ST_NOENUM_SUSPENDED         0x87

//USBͨ������
#define USB_CHANNEL_CDC 0
#define USB_CHANNEL_HID 1

//��ʼ��USBģ�鲢��������
void initUSB();

//��ȡUSB���ӵĵ�ǰ״̬
int USB_GetConnectionStatus();

//����ĳ��ͨ����ǰ�Ƿ������ݿɶ�
// channel - ͨ����
// ���� - 0:��, else:��,��ֵΪ���������ֽ���
int USB_ChannelCanRead(uint8_t channel);

//����ĳ��ͨ����ǰ�Ƿ���Է�������
// channel - ͨ����
// ���� - 0:���ɷ���, else:���Է���
int USB_ChannelCanWrite(uint8_t channel);

//��ȡĳ��ͨ����ǰ�յ�����������
// channel - ͨ����
// buffer - ���ݻ������׵�ַ
// bufferSize - ���ݻ�������󳤶�
// ���� - ʵ�ʶ�ȡ���ֽ���
uint16_t USB_ReadChannel(uint8_t channel, char* buffer, uint16_t bufferSize);

//д��ĳ��ͨ��(��������)
// channel - ͨ����
// data - �����������׵�ַ
// size - ���������ݳ���
// ���� - ʵ�ʽ��뷢�Ͷ��е��ֽ���(����ʧ��Ϊ0���ɹ�Ϊsize)
uint16_t USB_WriteChannel(uint8_t channel, char* data, uint16_t size);

#include "dr_usb_hid.h"

//���̷���һ������
// modifier - ���ΰ���(��Ctrl, ��Shift��)��������dr_usb_hid.h��
// key - ������������dr_usb_hid.h��
// ���� - �Ƿ��ͳɹ�
int USB_SendKeyboardKey(uint8_t modifier, uint8_t key);

//����һ��������
// buttons - 3����갴��
// dX - Xλ��
// dY - Yλ��
// dZ - ����λ��
// ���� - �Ƿ��ͳɹ�
int USB_SendMouseOperation(uint8_t buttons, int8_t dX, int8_t dY, int8_t dZ); 

#endif
