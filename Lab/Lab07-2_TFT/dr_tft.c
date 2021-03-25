/*
 * dr_tft.c
 *
 *  Created on: 2013-6-1
 *      Author: DengPihui
 */

#include <msp430.h>
#include "dr_tft.h"

//--------------P5.0---------------------------------
#define	LCD_CS_SET		P5OUT |= 0x01
#define 	LCD_CS_CLR		P5OUT &=~0x01
//--------------P5.2---------------------------------
#define 	LCD_RS_SET		P5OUT |= 0x04
#define 	LCD_RS_CLR		P5OUT &=~0x04
//--------------P1.1---------------------------------
#define 	LCD_RST_SET		P1OUT |= 0x02
#define 	LCD_RST_CLR		P1OUT &=~0x02

//--------------P8.4---------------------------------
#define 	LCD_SCL_SET		P8OUT |= 0x10
#define 	LCD_SCL_CLR		P8OUT &=~0x10
//--------------P8.5---------------------------------
#define 	LCD_SCI_SET		P8OUT |= 0x20
#define 	LCD_SCI_CLR		P8OUT &=~0x20

#define tft_send_and_wait(x, y) tft_SendCmd(x, y);__delay_cycles(MCLK_FREQ / 1000);
#define tft_send_and_wait2(x, y, z) tft_SendCmd(x, y);__delay_cycles(MCLK_FREQ / 1000 * z);

//初始化TFT
void initTFT()
{
  //初始化RS、CS端口
  P5DIR |= BIT0 + BIT2;
  P1DIR |= BIT1;

  //LCD背光
  P3DIR |= BIT3;
  P3OUT &= ~BIT3;

  //复位TFTLCD
  LCD_RST_CLR;
  __delay_cycles(200); //20MHz下也能保证10us的延时
  LCD_RST_SET;
  __delay_cycles(MCLK_FREQ / 1000 * 100); //延时100ms
  LCD_CS_SET; //不用时关掉片选，有利于SPI容错性

  //配置与TFTLCD通信的串口
  UCB1CTL1 |= UCSWRST;
  UCB1CTL0 = UCCKPL + UCMSB + UCMST + UCSYNC; //下降沿变数据、上升沿采样；高位先；8位模式；主机；3线；同步
  UCB1CTL1 = UCSSEL__SMCLK + UCSWRST;
  UCB1BRW = SMCLK_FREQ / SPI_FREQ;
  P8REN |= BIT6;
  P8OUT &= ~BIT6;
  P8SEL |= BIT4 + BIT5 + BIT6;
  P8DIR |= BIT4 + BIT5;
  UCB1CTL1 &= ~UCSWRST;

  //写入初始化TFTLCD的指令
  tft_send_and_wait(TFTREG_SOFT_RESET, 1); //软件复位
  tft_send_and_wait2(TFTREG_OSC_CTL, 1, 10); //启动内部晶振

  //y-setting
  tft_send_and_wait(0x0024,0x007B);
  tft_send_and_wait(0x0025,0x003B);
  tft_send_and_wait(0x0026,0x0034);
  tft_send_and_wait(0x0027,0x0004);
  tft_send_and_wait(0x0052,0x0025);
  tft_send_and_wait(0x0053,0x0033);
  tft_send_and_wait(0x0061,0x001C);
  tft_send_and_wait(0x0062,0x002C);
  tft_send_and_wait(0x0063,0x0022);
  tft_send_and_wait(0x0064,0x0027);
  tft_send_and_wait(0x0065,0x0014);
  tft_send_and_wait(0x0066,0x0010);

  //Basical clock for 1 line (BASECOUNT[7:0]) number specified
  tft_send_and_wait(0x002E,0x002D);

  //Power supply setting
  tft_send_and_wait2(0x0019, 0x0000, 20);
  tft_send_and_wait(0x001A,0x1000);
  tft_send_and_wait(0x001B,0x0023);
  tft_send_and_wait(0x001C,0x0C01);
  tft_send_and_wait(0x001D,0x0000);
  tft_send_and_wait(0x001E,0x0009);
  tft_send_and_wait(0x001F,0x0035);
  tft_send_and_wait(0x0020,0x0015);
  tft_send_and_wait(0x0018,0x1E7B);

  //LCD display area setting
  tft_send_and_wait(TFTREG_DISPLAY_MINX,0x0000);
  tft_send_and_wait(TFTREG_DISPLAY_MINY,0x0000);
  tft_send_and_wait(TFTREG_DISPLAY_MAXX,0x00EF);
  tft_send_and_wait(TFTREG_DISPLAY_MAXY,0x013F);

  tft_send_and_wait(TFTREG_GATE_SCN, 2); //设定扫描方式
  tft_send_and_wait(TFTREG_GATE_EN, 0x0001); //将门置于正常输出状态
  tft_send_and_wait(TFTREG_RAM_CTL, TFTCMD_WINMODE + TFTCMD_RAM_YMODE); //开启window模式，Y方向计数(屏幕是横的)
  tft_send_and_wait(TFTREG_DISPLAY_CTL2, TFTCMD_XADDR_INV); //反转X地址
  tft_send_and_wait(TFTREG_DISPLAY_CTL1, 0); //进入正常输出模式
}

void tft_AddTxData(uint16_t val)
{
  while(!(UCB1IFG & UCTXIFG)); //等待发送缓冲区空
  UCB1TXBUF = (val >> 8) & 0xFF; //发送高位
  while(!(UCB1IFG & UCTXIFG)); //等待发送缓冲区空
  UCB1TXBUF = val & 0xFF; //发送低位
  while(UCB1STAT & UCBUSY); //等待最后一位实际送出
}

//向TFT屏发送一个地址，返回是否发送成功
int tft_SendIndex(uint16_t val)
{
  LCD_CS_CLR;
  LCD_RS_CLR;
  tft_AddTxData(val);
  LCD_CS_SET;
  return 1;
}

//向TFT屏发送一个数据，返回是否发送成功
int tft_SendData(uint16_t val)
{
  LCD_CS_CLR;
  LCD_RS_SET;
  tft_AddTxData(val);
  LCD_CS_SET;
  return 1;
}

//向TFT屏的寄存器reg发送数据data，返回是否发送成功
int tft_SendCmd(uint16_t reg, uint16_t data)
{
  tft_SendIndex(reg);
  tft_SendData(data);
  return 1;
}



