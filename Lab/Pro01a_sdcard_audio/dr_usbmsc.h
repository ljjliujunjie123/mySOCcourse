#ifndef __DR_USBMSC_H_
#define __DR_USBMSC_H_

//USB连接状态标识，与usblib430一致
#define ST_USB_DISCONNECTED         0x80
#define ST_USB_CONNECTED_NO_ENUM    0x81
#define ST_ENUM_IN_PROGRESS         0x82
#define ST_ENUM_ACTIVE              0x83
#define ST_ENUM_SUSPENDED           0x84
#define ST_FAILED_ENUM              0x85
#define ST_ERROR                    0x86
#define ST_NOENUM_SUSPENDED         0x87

//初始化U盘接口
void initUSBMSC();

//处理U盘空闲时任务，应当在空闲时调用该函数，但不可连续调用
int usbmsc_IdleTask();

//获取USB连接的当前状态
int usbmsc_GetConnectionStatus();

#endif
