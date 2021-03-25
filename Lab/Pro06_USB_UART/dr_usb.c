#include "dr_usb.h"
#include "dr_usb/descriptors.h"
#include "dr_usb/types.h"
#include "dr_usb/usbConstructs.h"
#include "dr_usb/usb.h"
#include "dr_usb/UsbCdc.h"
#include "dr_usb/UsbHid.h"

typedef enum
{
  typeCDC, typeHIDData
} USBChannelType;

typedef struct
{
  USBChannelType type;
  BYTE interfaceNumber;
} USBChannel;

#define USB_CHANNEL_COUNT 2
USBChannel usbChannels[USB_CHANNEL_COUNT] = 
{
  typeCDC, CDC0_INTFNUM,
  typeHIDData, HID0_INTFNUM
};

#define KEYBOARD_INTFNUM 2
#define MOUSE_INTFNUM    3

void initUSB()
{
  //初始化USB
  USB_init();
  
  //启用USB事件
  USB_setEnabledEvents(kUSB_VbusOnEvent + kUSB_VbusOffEvent + 
      kUSB_receiveCompletedEvent + kUSB_dataReceivedEvent + 
      kUSB_UsbSuspendEvent + kUSB_UsbResumeEvent + kUSB_UsbResetEvent);

  //检查是否连接上，若连接上触发相应事件(事件中进行与主机的连接)
  if (USB_connectionInfo() & kUSB_vbusPresent){
      USB_handleVbusOnEvent();
  }
}

int USB_GetConnectionStatus()
{
  return USB_connectionState();
}

int USB_ChannelCanRead(uint8_t channel)
{
  if(channel >= USB_CHANNEL_COUNT)
    return 0;
  
  USBChannel *chnl = usbChannels + channel;
  switch(chnl->type)
  {
  case typeCDC:
    {
      return USBCDC_bytesInUSBBuffer(chnl->interfaceNumber);
    }
    break;
  case typeHIDData:
    {
      return USBHID_bytesInUSBBuffer(chnl->interfaceNumber);
    }
    break;
  default:
    return 0;
  }
}

int USB_ChannelCanWrite(uint8_t channel)
{
  if(channel >= USB_CHANNEL_COUNT)
    return 0;
  
  USBChannel *chnl = usbChannels + channel;
  uint16_t flag;
  switch(chnl->type)
  {
  case typeCDC:
    {
      if(USBCDC_intfStatus(chnl->interfaceNumber, &flag, &flag) & 
         (kUSBCDC_waitingForSend + kUSBCDC_busNotAvailable))
        return 0;
      else
        return 1;
    }
    break;
  case typeHIDData:
    {
      if(USBHID_intfStatus(chnl->interfaceNumber, &flag, &flag) & 
         (kUSBHID_waitingForSend + kUSBHID_busNotAvailable))
        return 0;
      else
        return 1;
    }
    break;
  default:
    return 0;
  }
}

uint16_t USB_ReadChannel(uint8_t channel, char* buffer, uint16_t bufferSize)
{
  if(channel >= USB_CHANNEL_COUNT)
    return 0;
  
  USBChannel *chnl = usbChannels + channel;
  switch(chnl->type)
  {
  case typeCDC:
    {
      uint16_t recvDataSize;
      recvDataSize = USBCDC_bytesInUSBBuffer(chnl->interfaceNumber);
      if(recvDataSize == 0)
        return 0;
      if(recvDataSize > bufferSize)
        recvDataSize = bufferSize;
      if(USBCDC_receiveData((unsigned char*)buffer, recvDataSize, chnl->interfaceNumber)
         & kUSBCDC_receiveCompleted)
        return recvDataSize;
      else
        return 0;
    }  
    break;
  case typeHIDData:
    {
      uint16_t recvDataSize;
      recvDataSize = USBHID_bytesInUSBBuffer(chnl->interfaceNumber);
      if(recvDataSize == 0)
        return 0;
      if(recvDataSize > bufferSize)
        recvDataSize = bufferSize;
      if(USBHID_receiveData((unsigned char*)buffer, recvDataSize, chnl->interfaceNumber)
         & kUSBHID_receiveCompleted)
        return recvDataSize;
      else
        return 0;
    }
    break;
  default:
    return 0;
  }
}

uint16_t USB_WriteChannel(uint8_t channel, char* data, uint16_t size)
{
  if(!USB_ChannelCanWrite(channel))
    return 0;
  
  USBChannel *chnl = usbChannels + channel;
  switch(chnl->type)
  {
  case typeCDC:
    {
      if(USBCDC_sendData((unsigned char*)data, size, chnl->interfaceNumber)
         & kUSBCDC_sendStarted)
        return size;
      else
        return 0;
    }  
    break;
  case typeHIDData:
    {
      if(USBHID_sendData((unsigned char*)data, size, chnl->interfaceNumber)
         & kUSBHID_sendStarted)
        return size;
      else
        return 0;
    }
    break;
  default:
    return 0;
  }
}

int USB_SendKeyboardKey(uint8_t modifier, uint8_t key)
{
  static char report[8];
  report[0] = modifier;
  report[1] = 0x00;
  report[2] = key;
  report[3] = 0x00;
  report[4] = 0x00;
  report[5] = 0x00;
  report[6] = 0x00;
  report[7] = 0x00;
  if(USBHID_sendReport((void*)&report, KEYBOARD_INTFNUM) == kUSBHID_sendComplete)
    return 1;
  else
    return 0;
}

int USB_SendMouseOperation(uint8_t buttons, int8_t dX, int8_t dY, int8_t dZ)
{
  static char report[4];
  report[0] = buttons;
  report[1] = dX;
  report[2] = dY;
  report[3] = dZ;
  if(USBHID_sendReport((void*)&report, MOUSE_INTFNUM) == kUSBHID_sendComplete)
    return 1;
  else
    return 0;
}
