/*
 * dr_tft.h
 *
 *  Created on: 2013-6-1
 *      Author: DengPihui
 */

#ifndef DR_TFT_H_
#define DR_TFT_H_

#include <stdint.h>

#ifndef MCLK_FREQ
  #define MCLK_FREQ 20000000
#endif

#ifndef SMCLK_FREQ
  #define SMCLK_FREQ 20000000
#endif

#define SPI_FREQ 10000000

#define TFT_XSIZE 240
#define TFT_YSIZE 320

#define TFTREG_RAM_XADDR  0x0006
#define TFTREG_RAM_YADDR  0x0007
#define TFTREG_RAM_ACCESS 0x000E

#define TFTREG_DISPLAY_CTL1 0x0000
#define TFTREG_DISPLAY_CTL2 0x0001
#define TFTREG_SOFT_RESET 0x0003
#define TFTREG_OSC_CTL    0x003A
#define TFTREG_GATE_SCN   0x0032
#define TFTREG_GATE_EN    0x003B
#define TFTREG_RAM_CTL    0x0005

#define TFTREG_WIN_MINX   0x0008
#define TFTREG_WIN_MAXX   0x0009
#define TFTREG_WIN_MINY   0x000A
#define TFTREG_WIN_MAXY   0x000B

#define TFTREG_DISPLAY_MINX   0x0029
#define TFTREG_DISPLAY_MAXX   0x002B
#define TFTREG_DISPLAY_MINY   0x002A
#define TFTREG_DISPLAY_MAXY   0x002C

//RAM_CTL
#define TFTCMD_WINMODE    0x0010 //选择是否使用window模式
#define TFTCMD_RAMINV     0x0008 //是否反转RAM的IO数据
#define TFTCMD_RAM_YMODE  0x0004 //RAM地址计数按X方向还是Y方向

//DISPLAY_CTL1

//DISPLAY_CTL2
#define TFTCMD_XADDR_INV  0x0080 //反转X地址
#define TFTCMD_YADDR_INV  0x0040 //反转Y地址

/* TFT屏底层接口 */

//初始化TFT
void initTFT();

//向TFT屏发送一个地址，返回是否发送成功
int tft_SendIndex(uint16_t val);

//向TFT屏发送一个数据，返回是否发送成功
int tft_SendData(uint16_t val);

//向TFT屏的寄存器reg发送数据data，返回是否发送成功
int tft_SendCmd(uint16_t reg, uint16_t data);

/* TFT屏高层接口 */
/* 所有高层接口内置X、Y对调，即接口处X为横Y为纵 */

//将0~255表示的RGB颜色转换为TFT屏幕使用的颜色
static inline uint16_t etft_Color(uint8_t r, uint8_t g, uint8_t b)
{
  uint16_t temp = 0;
  temp |= (r << 8) & 0xF800;
  temp |= (g << 3) & 0x07E0;
  temp |= (b >> 3) & 0x001F;
  return temp;
}

//将一个区域置为某个颜色
void etft_AreaSet(uint16_t startX, uint16_t startY, uint16_t endX, uint16_t endY, uint16_t color);

//在指定的位置显示一个字符串
void etft_DisplayString(const char* str, uint16_t sx, uint16_t sy, uint16_t fRGB, uint16_t bRGB);

//在指定的位置显示一幅图片，image以24位位图数据区表示
//即像素顺序从左到右、从下到上(即行顺序倒转)，每3字节一个像素，顺序为B、G、R，每行字节数用0补齐至4的整倍数
//对常见24位位图，从0x36复制到文件末尾即可
void etft_DisplayImage(const uint8_t* image, uint16_t sx, uint16_t sy, uint16_t width, uint16_t height);




#endif /* DR_TFT_H_ */
