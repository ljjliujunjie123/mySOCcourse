/*
 * dr_tft2.c
 *
 *  Created on: 2013-6-1
 *      Author: DengPihui
 */
#include <msp430.h>
#include "dr_tft.h"
#include "dr_tft_ascii.h"

void etft_AreaSet(uint16_t startX, uint16_t startY, uint16_t endX, uint16_t endY, uint16_t color)
{
  uint16_t i,j;
  tft_SendCmd(TFTREG_WIN_MINX, startY);
  tft_SendCmd(TFTREG_WIN_MINY, startX);
  tft_SendCmd(TFTREG_WIN_MAXX, endY);
  tft_SendCmd(TFTREG_WIN_MAXY, endX);

  tft_SendCmd(TFTREG_RAM_XADDR, startY);
  tft_SendCmd(TFTREG_RAM_YADDR, startX);

  tft_SendIndex(TFTREG_RAM_ACCESS);
  for(i=0;i<endY - startY + 1;i++)
  {
    for(j=0;j<endX - startX + 1;j++)
    {
      tft_SendData(color);
    }
  }
}

void etft_DisplayString(const char* str, uint16_t sx, uint16_t sy, uint16_t fRGB, uint16_t bRGB)
{
  uint16_t cc = 0;
  uint16_t cx, cy;

  while(1)
  {
    char curchar = str[cc];
    if(curchar == '\0') //字符串已发送完
      return;

    cx = 0;
    cy = 0;
    //屏幕是横的，XY要对调
    tft_SendCmd(TFTREG_WIN_MINY, sx);//x start point
    tft_SendCmd(TFTREG_WIN_MINX, sy);//y start point
    tft_SendCmd(TFTREG_WIN_MAXY, sx+7);//x end point
    tft_SendCmd(TFTREG_WIN_MAXX, sy+15);//y end point
    tft_SendCmd(TFTREG_RAM_YADDR, sx);//x start point
    tft_SendCmd(TFTREG_RAM_XADDR, sy);//y start point
    tft_SendIndex(TFTREG_RAM_ACCESS);

    uint16_t color;
    while(1)
    {
      if(cx >= 8)
      {
        cx = 0;
        cy++;
        if(cy >= 16)
        { //一个字符发送完毕
          cc++; //下一个字符
          sx+=8;
          if(sx >= TFT_YSIZE) //越过行末
          {
            sx = 0;
            sy += 16;
          }
          break;
        }
      }

      if((tft_ascii[curchar*16 + cy] << cx) & 0x80)
        color = fRGB;
      else
        color = bRGB;

      tft_SendData(color);
      cx++; //X自增
    }
  }
}

void etft_DisplayImage(const uint8_t* image, uint16_t sx, uint16_t sy, uint16_t width, uint16_t height)
{
  uint16_t i,j;
  uint32_t row_length = width * 3; //每行像素数乘3
  if(row_length & 0x3) //非4整倍数
  {
    row_length |= 0x03;
    row_length += 1;
  }
  const uint8_t *ptr = image + (height - 1) * row_length;
  tft_SendCmd(TFTREG_WIN_MINX, sy);
  tft_SendCmd(TFTREG_WIN_MINY, sx);
  tft_SendCmd(TFTREG_WIN_MAXX, sy + height - 1);
  tft_SendCmd(TFTREG_WIN_MAXY, sx + width - 1);

  tft_SendCmd(TFTREG_RAM_XADDR, sy);
  tft_SendCmd(TFTREG_RAM_YADDR, sx);

  tft_SendIndex(TFTREG_RAM_ACCESS);
  for(i=0;i<height;i++)
  {
    for(j=0;j<width;j++)
    {
      tft_SendData(etft_Color(ptr[2], ptr[1], ptr[0]));
      ptr += 3;
    }
    ptr -= width * 3 + row_length;
  }
}




