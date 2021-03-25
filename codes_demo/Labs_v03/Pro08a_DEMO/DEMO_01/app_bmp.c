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
          continue; //����ļ���չ��

        filecount++;
        etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));
        if(bmp_DisplayBmpFile(fno.fname)) //�ļ�������ʾ�ɹ�
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
          else if(btn != 0) //�а�������
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
          continue; //����ļ���չ��

        filecount++;
        etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));
        if(bmp_DisplayBmpFile("F6638EVM.bmp")) //�ļ�������ʾ�ɹ�
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
          continue; //����ļ���չ��

        filecount++;
        etft_AreaSet(0,0,319,239,etft_Color(128, 128, 128));
        if(bmp_DisplayBmpFile(filename)) //�ļ�������ʾ�ɹ�
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
