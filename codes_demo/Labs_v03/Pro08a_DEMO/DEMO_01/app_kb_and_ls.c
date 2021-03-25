#include "app_kb_and_ls.h"
#include <stdint.h>
#include <msp430.h>
#include "dr_i2c.h"

int appkb_scan = 0;

#define SEG_ADDR 0x20
#define SEG_CS 2
#define SEG_SS 3
#define SEG_DIR0 6
#define SEG_DIR1 7
#define KB_ADDR 0x21
#define KB_IN 0
#define KB_OUT 1
#define KB_PIR 2
#define KB_DIR 3

extern const uint8_t SEG_CTRL_BIN[17];

const int KEYBOARD_VALUE[16] =
{
  15, 14, 13, 12, 11, 10, 0, 9,
  8, 7, 6, 5, 4, 3, 2, 1
};

uint8_t seg_display = 0, kb_line = 0;
uint16_t cur_kb_input = 0xFFFF, last_kb_input = 0xFFFF;
uint8_t seg_buffer[8] =
{
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07
};

int kb_in; //保存检查键盘输入用的I2C查询代号

void app_kb_callback_scanner()
{
  if(!appkb_scan)
    return;

  static int cs = 0;
  I2C_RequestSend(SEG_ADDR, SEG_SS, 0);
  I2C_RequestSend(SEG_ADDR, SEG_CS, 1 << cs);
  I2C_RequestSend(SEG_ADDR, SEG_SS, seg_buffer[cs]);
  cs++;
  if(cs >= 8)
    cs = 0;

  static int count = 0;
  static int kbcount = 0;

  kbcount++;
  if(kbcount == 5)
    I2C_CheckQuery(kb_in); //空读取，迫使I2C模块更新数据
  if(kbcount > 10)
  {
    kbcount = 0;

    cur_kb_input &= ~(0xF << (kb_line * 4) );
    cur_kb_input |= ((I2C_CheckQuery(kb_in) & 0xF) << (kb_line * 4));

    uint16_t temp = (cur_kb_input ^ last_kb_input) & last_kb_input; //找出向下跳变的按键
    int i;
    for(i=0;i<16;++i)
    {
      if(temp & (1<<i))
      {
        seg_display = KEYBOARD_VALUE[i];
        count = 0;
        break;
      }
    }
    last_kb_input = cur_kb_input;

    kb_line++;
    if(kb_line >= 4)
    kb_line = 0;

    I2C_RequestSend(KB_ADDR, KB_OUT, ~(1 << (kb_line + 4)) );
  }

  count++;
  if(count > 1500)
  {
    count = 0;

    seg_display++;
    if(seg_display >= 16)
      seg_display = 0;
  }
}

void app_keyboard()
{
  initI2C();

  etft_AreaSet(0,0,319,239,0);

  etft_DisplayString("Press any key to exit...",40,80,65504,0);

  //设置数码管的IO扩展器端口方向
  I2C_RequestSend(SEG_ADDR, SEG_DIR0, 0);
  I2C_RequestSend(SEG_ADDR, SEG_DIR1, 0);
  //设置矩阵键盘的IO扩展器端口方向
  I2C_RequestSend(KB_ADDR, KB_DIR, 0x0F);
  //设置矩阵键盘的初始扫描线
  I2C_RequestSend(KB_ADDR, KB_OUT, ~(1 << (kb_line + 4)) );
  //要求自动查询键盘输入
  kb_in = I2C_AddRegQuery(KB_ADDR, KB_IN);

  led_SetLed(4,1); //这个灯在复位口上

  appkb_scan = 1;
  while(1)
  {
    int i, j;
    for(i=seg_display, j=0;j<8;++j)
    {
      seg_buffer[j] = SEG_CTRL_BIN[i];
      i++;
      if(i >= 16)
        i = 0;
    }

    if(btn_GetBtnInput(0))
      break;
  }
  appkb_scan = 0;

  I2C_RequestSend(SEG_ADDR, SEG_SS, 0);
  __delay_cycles(20000000 / 2);

  UCB0CTL1 |= UCSWRST;
  UCB0IE &= ~UCNACKIE;
  led_SetLed(4,0);
}
