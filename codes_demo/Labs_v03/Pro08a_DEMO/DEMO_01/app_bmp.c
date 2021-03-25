#include <msp430.h>
#include "dr_sdcard.h"
#include "TFT\dr_tft.h"
#include <string.h>
#include <stdio.h>
#include "key_led\dr_buttons_and_leds.h"

#define FBUF_SIZE 320*3
uint8_t fbuf[FBUF_SIZE];

typedef struct tagBITMAPFILEHEADER
{
  WORD bfType; // 位图文件的类型，必须为BM(1-2字节）
  DWORD bfSize; // 位图文件的大小，以字节为单位（3-6字节）
  WORD bfReserved1; // 位图文件保留字，必须为0(7-8字节）
  WORD bfReserved2; // 位图文件保留字，必须为0(9-10字节）
  DWORD bfOffBits; // 位图数据的起始位置，以相对于位图（11-14字节）
                   // 文件头的偏移量表示，以字节为单位
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
  DWORD biSize; // 本结构所占用字节数（15-18字节）
  LONG biWidth; // 位图的宽度，以像素为单位（19-22字节）
  LONG biHeight; // 位图的高度，以像素为单位（23-26字节）
  WORD biPlanes; // 目标设备的级别，必须为1(27-28字节）
  WORD biBitCount;// 每个像素所需的位数，必须是1（双色），（29-30字节）
                  // 4(16色），8(256色）或24（真彩色）之一
  DWORD biCompression; // 位图压缩类型，必须是 0（不压缩），（31-34字节）
                       // 1(BI_RLE8压缩类型）或2(BI_RLE4压缩类型）之一
  DWORD biSizeImage; // 位图的大小，以字节为单位（35-38字节）
  LONG biXPelsPerMeter; // 位图水平分辨率，每米像素数（39-42字节）
  LONG biYPelsPerMeter; // 位图垂直分辨率，每米像素数（43-46字节)
  DWORD biClrUsed;// 位图实际使用的颜色表中的颜色数（47-50字节）
  DWORD biClrImportant;// 位图显示过程中重要的颜色数（51-54字节）
} BITMAPINFOHEADER;

BITMAPFILEHEADER _bmpFileHeader;
BITMAPINFOHEADER _bmpInfo;

int bmp_CheckHeader(BITMAPFILEHEADER *h)
{
  if(h->bfType != 19778) //'BM'
    return 0;
  if(h->bfReserved1 != 0 || h->bfReserved2 != 0)
    return 0;
  return 1;
}

int bmp_CheckFormat(BITMAPINFOHEADER *i)
{
  if(i->biSize != 40)
    return 0;
  if(i->biBitCount != 24) //不支持24位图以外的格式
    return 0;
  if(i->biCompression != 0) //不支持压缩
    return 0;
  if(i->biWidth > 320) //太宽
    return 0;
  if(i->biHeight > 240) //太高
    return 0;
  return 1;
}

int bmp_DisplayBmpFile(char *filename)
{
  FRESULT rc;
  FIL fil; /* File object */
  uint16_t size;

  rc = f_open(&fil, filename, FA_READ); //打开文件
  rc = f_read(&fil, &_bmpFileHeader, sizeof(_bmpFileHeader), &size); //读取文件头
  if (rc || size != sizeof(_bmpFileHeader)) /* Error or end of file */
  {
    f_close(&fil); //关闭文件
    return 0;
  }
  if (!bmp_CheckHeader(&_bmpFileHeader)) //检查文件头格式
  {
    f_close(&fil); //关闭文件
    return 0;
  }

  rc = f_read(&fil, &_bmpInfo, sizeof(_bmpInfo), &size); //读取文件信息
  if (rc || size != sizeof(_bmpInfo)) /* Error or end of file */
  {
    f_close(&fil); //关闭文件
    return 0;
  }
  if (!bmp_CheckFormat(&_bmpInfo)) //检查文件格式
  {
    f_close(&fil); //关闭文件
    return 0;
  }

  uint32_t row_length = _bmpInfo.biWidth * 3; //每行像素数乘3
  if(row_length & 0x3) //非4整倍数时补齐
  {
    row_length |= 0x03;
    row_length += 1;
  }

  int r, rcnt = _bmpInfo.biHeight;
  if(rcnt > 0) //倒转位图行顺序由下到上
    r = rcnt - 1;
  else
  { //行顺序由上到下，少见
    r = 0;
    rcnt = -rcnt;
  }

  while(1)
  {
    rc = f_read(&fil, &fbuf, row_length, &size); //读取一行的数据
    if (rc || size != row_length) /* Error or end of file */
    {
      f_close(&fil); //关闭文件
      return 0;
    }
    etft_DisplayImage(fbuf, 0, r, _bmpInfo.biWidth, 1);
    if(_bmpInfo.biHeight > 0) //倒转位图
      r--;
    else
      r++;
    rcnt--;
    if(rcnt == 0)
      break;
  }

  f_close(&fil); //关闭文件
  return 1;
}

void strupr(char* str)
{
  char *ptr = str;
  while(!*ptr)
  {
    if(*ptr >= 'a' && *ptr <='z')
      *ptr -= 'a' - 'A';
    ptr++;
  }
}

//#include "app_icon.h"

extern volatile int32_t refClock;

void bmp_BmpViewerMain()
{
  FRESULT rc;   /* Result code */
  FATFS fatfs;  /* File system object */
  DIRS dir;     /* Directory object */
  FILINFO fno;  /* File information object */
  int filecount;
  int32_t last_pic_display_time;

  etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));

  //etft_DisplayImage(icon, 0, 0, 30, 30);
//  etft_DisplayString("BMP Viewer", 31, 0, etft_Color(0, 0, 255), etft_Color(128, 128, 128));

  while(1)
  {
    f_mount(0, &fatfs); /* Register volume work area (never fails) */
    rc = f_opendir(&dir, "");
    if(rc)
    {
      etft_DisplayString("ERROR: No SDCard detected!", 0, 0, etft_Color(0xFF,0,0), etft_Color(128, 128, 128));
      if(btn_GetBtnInput(1) == 5)
                  return;
    }

    if(btn_GetBtnInput(0) == 5) //S5
      return;

    filecount = 0;
    for (;;)
    {
      rc = f_readdir(&dir, &fno);                        // Read a directory item
      if (rc || !fno.fname[0])
      {
        if(filecount == 0)
        {
          etft_DisplayString("File not found!", 0, 0, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
          if(btn_GetBtnInput(1) == 5)
            return;
        }
        break;                    // Error or end of dir
      }
      if (fno.fattrib & AM_DIR)                          //this is a directory
      {

      }
      else                                               //this is a file
      {
        char temps[20];
        strcpy(temps, fno.fname);
        strupr(temps);

        if(strstr(temps, ".BMP") == 0)
          continue; //检查文件扩展名

        filecount++;
        etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));
        if(bmp_DisplayBmpFile(fno.fname)) //文件解析显示成功
        {
            last_pic_display_time = refClock;
        }
        else
        {
          //tft_DisplayImage(icon, 0, 0, 30, 30);
          etft_DisplayString("File not supported!", 0, 0, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
          etft_DisplayString(fno.fname, 0, 16, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
        }

        while(1)
        {
          int btn = btn_GetBtnInput(0);
          if(btn == 5) //S5
            return;
          else if(btn != 0) //有按键按下
            break;
          else if(refClock - last_pic_display_time > 3500)
            break;
        }
      }
    }
  }
}

void bmp_LogViewer()
{
  FRESULT rc;   /* Result code */
  FATFS fatfs;  /* File system object */
  DIRS dir;     /* Directory object */
  FILINFO fno;  /* File information object */
  int filecount;
  int32_t last_pic_display_time;

  etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));

  //etft_DisplayImage(icon, 0, 0, 30, 30);
//  etft_DisplayString("BMP Viewer", 31, 0, etft_Color(0, 0, 255), etft_Color(128, 128, 128));


    f_mount(0, &fatfs); /* Register volume work area (never fails) */
    rc = f_opendir(&dir, "");
    if(rc)
    {
      etft_DisplayString("ERROR: No SDCard detected!", 0, 0, etft_Color(0xFF,0,0), etft_Color(128, 128, 128));
      __delay_cycles(20000000);
      return;
    }

    if(btn_GetBtnInput(0) == 5) //S5
      return;

    filecount = 0;
    for (;;)
    {
      rc = f_readdir(&dir, &fno);                        // Read a directory item
      if (rc || !fno.fname[0])
      {
        if(filecount == 0)
        {
          etft_DisplayString("File not found!", 0, 0, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
          __delay_cycles(20000000);
            return;
        }
        break;                    // Error or end of dir
      }
      if (fno.fattrib & AM_DIR)                          //this is a directory
      {

      }
      else                                               //this is a file
      {
        char temps[20];
        strcpy(temps, fno.fname);
        strupr(temps);

        if(strstr(temps, ".BMP") == 0)
          continue; //检查文件扩展名

        filecount++;
        etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));
        if(bmp_DisplayBmpFile("F6638EVM.bmp")) //文件解析显示成功
        {
            last_pic_display_time = refClock;
        }
        else
        {
          //tft_DisplayImage(icon, 0, 0, 30, 30);
          etft_DisplayString("File not supported!", 0, 0, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
          etft_DisplayString(fno.fname, 0, 16, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
        }

        __delay_cycles(70000000);
        break;

      }
    }

}

void bmp_display(char *filename)
{
  FRESULT rc;   /* Result code */
  FATFS fatfs;  /* File system object */
  DIRS dir;     /* Directory object */
  FILINFO fno;  /* File information object */
  int filecount;
  int32_t last_pic_display_time;

  etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));

  //etft_DisplayImage(icon, 0, 0, 30, 30);
//  etft_DisplayString("BMP Viewer", 31, 0, etft_Color(0, 0, 255), etft_Color(128, 128, 128));


    f_mount(0, &fatfs); /* Register volume work area (never fails) */
    rc = f_opendir(&dir, "");
    if(rc)
    {
      etft_DisplayString("ERROR: No SDCard detected!", 0, 0, etft_Color(0xFF,0,0), etft_Color(128, 128, 128));
      __delay_cycles(20000000);
      return;
    }

    if(btn_GetBtnInput(0) == 5) //S5
      return;

    filecount = 0;
    for (;;)
    {
      rc = f_readdir(&dir, &fno);                        // Read a directory item
      if (rc || !fno.fname[0])
      {
        if(filecount == 0)
        {
          etft_DisplayString("File not found!", 0, 0, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
          __delay_cycles(20000000);
            return;
        }
        break;                    // Error or end of dir
      }
      if (fno.fattrib & AM_DIR)                          //this is a directory
      {

      }
      else                                               //this is a file
      {
        char temps[20];
        strcpy(temps, fno.fname);
        strupr(temps);

        if(strstr(temps, ".BMP") == 0)
          continue; //检查文件扩展名

        filecount++;
        etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));
        if(bmp_DisplayBmpFile(filename)) //文件解析显示成功
        {
            last_pic_display_time = refClock;
        }
        else
        {
          //tft_DisplayImage(icon, 0, 0, 30, 30);
          etft_DisplayString("File not supported!", 0, 0, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
          etft_DisplayString(fno.fname, 0, 16, etft_Color(255, 0, 0), etft_Color(128, 128, 128));
        }

        __delay_cycles(70000000);
        break;

      }
    }

}
