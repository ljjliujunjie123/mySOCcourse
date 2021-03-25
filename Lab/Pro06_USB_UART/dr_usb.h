#ifndef __DR_USB_H_
#define __DR_USB_H_

/********************************
  USB模块

!!!WinXP或Mac无法使用本模块的协议，连接后可能不识别

提供使用USB进行通讯的接口，同时提供CDC与HID两个通道(代码中为channel)
CDC通道在PC端建立虚拟串口，在PC端可以直接按串口访问
HID通道被PC端识别为键盘/鼠标，但通过TI的标准进行数据传输，在PC端需要使用TI的接口访问
两个通道在MSP430上的本模块中使用同一接口进行访问，通过通道号进行区分

仅使用USB_ReadChannel进行读取时需要注意，CDC模式仅有128B、HID模式仅有64B的内部缓冲区
若内部缓冲区被写满而数据未被读出，则可能的后续传输将被阻塞

使用USB_WriteChannel进行发送时，可以一次请求传输32K数据，但在发送操作结束前数据缓冲区应保持内容不变

CDC模式最高通讯速率788KB/s
HID模式最高通讯速率64KB/s
********************************/

#include <stdint.h>

//USB连接状态标识，与usblib430一致
#define ST_USB_DISCONNECTED         0x80
#define ST_USB_CONNECTED_NO_ENUM    0x81
#define ST_ENUM_IN_PROGRESS         0x82
#define ST_ENUM_ACTIVE              0x83
#define ST_ENUM_SUSPENDED           0x84
#define ST_FAILED_ENUM              0x85
#define ST_ERROR                    0x86
#define ST_NOENUM_SUSPENDED         0x87

//USB通道代号
#define USB_CHANNEL_CDC 0
#define USB_CHANNEL_HID 1

//初始化USB模块并连接主机
void initUSB();

//获取USB连接的当前状态
int USB_GetConnectionStatus();

//测试某个通道当前是否有数据可读
// channel - 通道号
// 返回 - 0:无, else:有,其值为缓冲区中字节数
int USB_ChannelCanRead(uint8_t channel);

//测试某个通道当前是否可以发送数据
// channel - 通道号
// 返回 - 0:不可发送, else:可以发送
int USB_ChannelCanWrite(uint8_t channel);

//读取某个通道当前收到的所有数据
// channel - 通道号
// buffer - 数据缓冲区首地址
// bufferSize - 数据缓冲区最大长度
// 返回 - 实际读取的字节数
uint16_t USB_ReadChannel(uint8_t channel, char* buffer, uint16_t bufferSize);

//写入某个通道(发送数据)
// channel - 通道号
// data - 待发送数据首地址
// size - 待发送数据长度
// 返回 - 实际进入发送队列的字节数(发送失败为0，成功为size)
uint16_t USB_WriteChannel(uint8_t channel, char* data, uint16_t size);

#include "dr_usb_hid.h"

//键盘发送一个按键
// modifier - 修饰按键(左Ctrl, 右Shift等)，定义在dr_usb_hid.h中
// key - 按键，定义在dr_usb_hid.h中
// 返回 - 是否发送成功
int USB_SendKeyboardKey(uint8_t modifier, uint8_t key);

//发送一个鼠标操作
// buttons - 3个鼠标按键
// dX - X位移
// dY - Y位移
// dZ - 滚轮位移
// 返回 - 是否发送成功
int USB_SendMouseOperation(uint8_t buttons, int8_t dX, int8_t dY, int8_t dZ); 

#endif
