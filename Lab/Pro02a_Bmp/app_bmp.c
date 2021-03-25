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
  WORD bfType; // λͼ�ļ������ͣ�����ΪBM(1-2�ֽڣ�
  DWORD bfSize; // λͼ�ļ��Ĵ�С�����ֽ�Ϊ��λ��3-6�ֽڣ�
  WORD bfReserved1; // λͼ�ļ������֣�����Ϊ0(7-8�ֽڣ�
  WORD bfReserved2; // λͼ�ļ������֣�����Ϊ0(9-10�ֽڣ�
  DWORD bfOffBits; // λͼ���ݵ���ʼλ�ã��������λͼ��11-14�ֽڣ�
                   // �ļ�ͷ��ƫ������ʾ�����ֽ�Ϊ��λ
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER{
  DWORD biSize; // ���ṹ��ռ���ֽ�����15-18�ֽڣ�
  LONG biWidth; // λͼ�Ŀ�ȣ�������Ϊ��λ��19-22�ֽڣ�
  LONG biHeight; // λͼ�ĸ߶ȣ�������Ϊ��λ��23-26�ֽڣ�
  WORD biPlanes; // Ŀ���豸�ļ��𣬱���Ϊ1(27-28�ֽڣ�
  WORD biBitCount;// ÿ�����������λ����������1��˫ɫ������29-30�ֽڣ�
                  // 4(16ɫ����8(256ɫ����24�����ɫ��֮һ
  DWORD biCompression; // λͼѹ�����ͣ������� 0����ѹ��������31-34�ֽڣ�
                       // 1(BI_RLE8ѹ�����ͣ���2(BI_RLE4ѹ�����ͣ�֮һ
  DWORD biSizeImage; // λͼ�Ĵ�С�����ֽ�Ϊ��λ��35-38�ֽڣ�
  LONG biXPelsPerMeter; // λͼˮƽ�ֱ��ʣ�ÿ����������39-42�ֽڣ�
  LONG biYPelsPerMeter; // λͼ��ֱ�ֱ��ʣ�ÿ����������43-46�ֽ�)
  DWORD biClrUsed;// λͼʵ��ʹ�õ���ɫ���е���ɫ����47-50�ֽڣ�
  DWORD biClrImportant;// λͼ��ʾ��������Ҫ����ɫ����51-54�ֽڣ�
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
  if(i->biBitCount != 24) //��֧��24λͼ����ĸ�ʽ
    return 0;
  if(i->biCompression != 0) //��֧��ѹ��
    return 0;
  if(i->biWidth > 320) //̫��
    return 0;
  if(i->biHeight > 240) //̫��
    return 0;
  return 1;
}

int bmp_DisplayBmpFile(char *filename)
{
  FRESULT rc;
  FIL fil; /* File object */
  uint16_t size;

  rc = f_open(&fil, filename, FA_READ); //���ļ�
  rc = f_read(&fil, &_bmpFileHeader, sizeof(_bmpFileHeader), &size); //��ȡ�ļ�ͷ
  if (rc || size != sizeof(_bmpFileHeader)) /* Error or end of file */
  {
    f_close(&fil); //�ر��ļ�
    return 0;
  }
  if (!bmp_CheckHeader(&_bmpFileHeader)) //����ļ�ͷ��ʽ
  {
    f_close(&fil); //�ر��ļ�
    return 0;
  }

  rc = f_read(&fil, &_bmpInfo, sizeof(_bmpInfo), &size); //��ȡ�ļ���Ϣ
  if (rc || size != sizeof(_bmpInfo)) /* Error or end of file */
  {
    f_close(&fil); //�ر��ļ�
    return 0;
  }
  if (!bmp_CheckFormat(&_bmpInfo)) //����ļ���ʽ
  {
    f_close(&fil); //�ر��ļ�
    return 0;
  }

  uint32_t row_length = _bmpInfo.biWidth * 3; //ÿ����������3
  if(row_length & 0x3) //��4������ʱ����
  {
    row_length |= 0x03;
    row_length += 1;
  }

  int r, rcnt = _bmpInfo.biHeight;
  if(rcnt > 0) //��תλͼ��˳�����µ���
    r = rcnt - 1;
  else
  { //��˳�����ϵ��£��ټ�
    r = 0;
    rcnt = -rcnt;
  }

  while(1)
  {
    rc = f_read(&fil, &fbuf, row_length, &size); //��ȡһ�е�����
    if (rc || size != row_length) /* Error or end of file */
    {
      f_close(&fil); //�ر��ļ�
      return 0;
    }
    etft_DisplayImage(fbuf, 0, r, _bmpInfo.biWidth, 1);
    if(_bmpInfo.biHeight > 0) //��תλͼ
      r--;
    else
      r++;
    rcnt--;
    if(rcnt == 0)
      break;
  }

  f_close(&fil); //�ر��ļ�
  return 1;
}

