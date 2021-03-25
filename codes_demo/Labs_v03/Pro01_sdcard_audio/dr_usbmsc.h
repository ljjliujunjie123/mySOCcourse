#ifndef __DR_USBMSC_H_
#define __DR_USBMSC_H_

//USB����״̬��ʶ����usblib430һ��
#define ST_USB_DISCONNECTED         0x80
#define ST_USB_CONNECTED_NO_ENUM    0x81
#define ST_ENUM_IN_PROGRESS         0x82
#define ST_ENUM_ACTIVE              0x83
#define ST_ENUM_SUSPENDED           0x84
#define ST_FAILED_ENUM              0x85
#define ST_ERROR                    0x86
#define ST_NOENUM_SUSPENDED         0x87

//��ʼ��U�̽ӿ�
void initUSBMSC();

//����U�̿���ʱ����Ӧ���ڿ���ʱ���øú�������������������
int usbmsc_IdleTask();

//��ȡUSB���ӵĵ�ǰ״̬
int usbmsc_GetConnectionStatus();

#endif
