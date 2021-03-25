#include <msp430.h>
#include "dr_usbmsc.h"
#include "dr_usbmsc/usb.h"
#include "dr_usbmsc/UsbMsc.h"
#include <stdint.h>
#include "dr_sdcard/ff.h"
#include "dr_sdcard/diskio.h"
#include "dr_sdcard/mmc.h"

const WORD BYTES_PER_BLOCK = 512;

struct config_struct USBMSC_config = {
    0x00,   //The number of this LUN.  (Ignored in this version of the API.)
    0x00,   //PDT (Peripheral Device Type). 0x00 = "direct-access block device", which includes all magnetic media.  (Ignored in
            //this version of the API.)
    0x80,   //Indicates whether the medium is removable.  0x00 = not removable, 0x80 = removable
    "TI",   //T10 Vendor ID.  Generally not enforced, and has no particular impact on most operating systems.
    "MSC",  //T10 Product ID.  Selected by the owner of the vendor ID above.  Has no impact on most OSes.
    "v3.0", //T10 revision string.  Selected by the owner of the vendor ID above.  Has no impact on most OSes.
};

uint8_t RWbuf[512];
USBMSC_RWbuf_Info *RWbuf_info;

struct USBMSC_mediaInfoStr mediaInfo;

void initUSBMSC()
{
  disk_initialize(0);
  
  USB_init();
  
  USB_setEnabledEvents(kUSB_allUsbEvents);
  
  RWbuf_info = USBMSC_fetchInfoStruct();

  //USBMSC_updateMediaInfo() must be called prior to USB connection.  We check if the card is present, and if so, pull its size
  //and bytes per block.
  if (detectCard()){
      mediaInfo.mediaPresent = kUSBMSC_MEDIA_PRESENT;
  }
  else {
      mediaInfo.mediaPresent = kUSBMSC_MEDIA_NOT_PRESENT;
  }
  mediaInfo.mediaChanged = 0x00;
  mediaInfo.writeProtected = 0x00;
  disk_ioctl(0,GET_SECTOR_COUNT,&mediaInfo.lastBlockLba); //Returns the number of blocks (sectors) in the media.
  mediaInfo.bytesPerBlock = BYTES_PER_BLOCK;                          //Block size will always be 512
  USBMSC_updateMediaInfo(0, &mediaInfo);

  //The data interchange buffer (used when handling SCSI READ/WRITE) is declared by the application, and
  //registered with the API using this function.  This allows it to be assigned dynamically, giving
  //the application more control over memory management.
  USBMSC_registerBufInfo(0, &RWbuf[0], 0, sizeof(RWbuf));
  
  //If USB is already connected when the program starts up, then there won't be a USB_handleVbusOnEvent().
  //So we need to check for it, and manually connect if the host is already present.
  if (USB_connectionInfo() & kUSB_vbusPresent){
      if (USB_enable() == kUSB_succeed){
          USB_reset();
          USB_connect();
      }
  }
}

BYTE checkInsertionRemoval (void)
{
  BYTE newCardStatus = detectCard();                                  //Check card status -- there or not?

  if ((newCardStatus) &&
      (mediaInfo.mediaPresent == kUSBMSC_MEDIA_NOT_PRESENT)){
      //An insertion has been detected -- inform the API
      mediaInfo.mediaPresent = kUSBMSC_MEDIA_PRESENT;
      mediaInfo.mediaChanged = 0x01;
      DRESULT SDCard_result = disk_ioctl(0,
          GET_SECTOR_COUNT,
          &mediaInfo.lastBlockLba);                                   //Get the size of this new medium
      USBMSC_updateMediaInfo(0, &mediaInfo);
  }

  if ((!newCardStatus) && (mediaInfo.mediaPresent == kUSBMSC_MEDIA_PRESENT)){
      //A removal has been detected -- inform the API
      mediaInfo.mediaPresent = kUSBMSC_MEDIA_NOT_PRESENT;
      mediaInfo.mediaChanged = 0x01;
      USBMSC_updateMediaInfo(0, &mediaInfo);
  }

  return ( newCardStatus) ;
}

int usbmsc_IdleTask()
{
  if(USB_connectionState() == ST_ENUM_ACTIVE)
  {
    if(USBMSC_poll() == kUSBMSC_okToSleep) //无任务要处理
      return 0;
    
    while (RWbuf_info->operation == kUSBMSC_READ)
    {
                                                //A READ operation is underway, and the app has been requested to access
                                                //the medium.  So, call file system to read
                                                //to do so.  Note this is a low level FatFs call -- we are not
                                                //attempting to open a file ourselves.  The host is
                                                //in control of this access, we're just carrying it out.
        DRESULT dresult = disk_read(0,          //Physical drive number (0)
            RWbuf_info->bufferAddr,             //Pointer to the user buffer
            RWbuf_info->lba,                    //First LBA of this buffer operation
            RWbuf_info->lbCount);               //The number of blocks being requested as part of this operation

                                                //The result of the file system call needs to be communicated to the
                                                //host.  Different file system software uses
                                                //different return codes, but they all communicate the same types of
                                                //results.  This code ultimately gets passed to the
                                                //host app that issued the command to read (or if the user did it the
                                                //host OS, perhaps in a dialog box).
        switch (dresult)
        {
            case RES_OK:
                RWbuf_info->returnCode = kUSBMSC_RWSuccess;
                break;
            case RES_ERROR:                     //In FatFs, this result suggests the medium may have been removed
                                                //recently.
                if (!checkInsertionRemoval()){  //This application function checks for the SD-card, and if missing,
                                                //calls USBMSC_updateMediaInfo() to inform the API
                    RWbuf_info->returnCode =
                        kUSBMSC_RWMedNotPresent;
                }
                break;
            case RES_NOTRDY:
                RWbuf_info->returnCode = kUSBMSC_RWNotReady;
                break;
            case RES_PARERR:
                RWbuf_info->returnCode = kUSBMSC_RWLbaOutOfRange;
                break;
        }
        USBMSC_bufferProcessed();
    }

                                                //Everything in this section is analogous to READs.  Reference the
                                                //comments above.
    while (RWbuf_info->operation == kUSBMSC_WRITE)
    {
        DRESULT dresult = disk_write(0,         //Physical drive number (0)
            RWbuf_info->bufferAddr,             //Pointer to the user buffer
            RWbuf_info->lba,                    //First LBA of this buffer operation
            RWbuf_info->lbCount);               //The number of blocks being requested as part of this operation
        switch (dresult)
        {
            case RES_OK:
                RWbuf_info->returnCode = kUSBMSC_RWSuccess;
                break;
            case RES_ERROR:
                if (!checkInsertionRemoval()){
                    RWbuf_info->returnCode =
                        kUSBMSC_RWMedNotPresent;
                }
                break;
            case RES_NOTRDY:
                RWbuf_info->returnCode = kUSBMSC_RWNotReady;
                break;
            case RES_PARERR:
                RWbuf_info->returnCode = kUSBMSC_RWLbaOutOfRange;
                break;
            default:
                RWbuf_info->returnCode = kUSBMSC_RWNotReady;
                break;
        }
        USBMSC_bufferProcessed();
    }
    
    return 1;
  }
  else
  {
    return 0;
  }
}

int usbmsc_GetConnectionStatus()
{
  return USB_connectionState();
}

