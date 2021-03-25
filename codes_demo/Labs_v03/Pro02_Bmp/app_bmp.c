#include <msp430.h>
#include "dr_sdcard.h"
#include "dr_tft.h"
#include "dr_usbmsc.h"
#include <string.h>
#include <stdio.h>

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

