/*
 * main.c
 */
#include <msp430f6638.h>

/*************************** 宏定义 ******************************/

typedef unsigned int uint;
typedef unsigned char uchar;

#define LEN 6 //宏定义缓冲区大小
#define ADR 0x1880//宏定义写入Flash的起始地址，位于Segment C
#define BYTE 0//单字节写入标志
#define WORD 1//双字节写入标志
#define DELETE 0//清除值

//宏定义相应寄存器的地址
#define P4_OUT 0x0223
#define P4_DIR 0x0225
#define P5_OUT 0X0242
#define P5_DIR 0x0244
#define P8_OUT 0x0263
#define P8_DIR 0x0265

/************************** 函数申明  *****************************/

void FlashWriteByte(uint address,uchar *byte,uint count);
void FlashWriteWord(uint address,uint *word,uint count);
void FlashErase(uint address,uint mode,uint length);
void FlashRead(uint address,uint *buf,uint mode,uint length);

/*************************** 主函数  ******************************/

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;//关闭看门狗
	//uchar BSetBuf[LEN] = {0}; 单个字节写入缓冲区，例子中未用到
	uint WSetBuf[LEN] = {P4_DIR,P5_DIR,P8_DIR,P4_OUT,P5_OUT,P8_OUT};
	// 双字节写入缓冲区
	uint RxBuf[LEN] = {0};//数据读取缓冲区
	FlashErase(ADR,WORD,LEN);//擦除写入Flash中的数据
	FlashWriteWord(ADR,WSetBuf,LEN);//双字节写入函数
	FlashRead(ADR,RxBuf,WORD,LEN);//读取函数

	/*取得缓冲区的数据作为地址并对其赋值，此处设置点亮LED灯的GPIO管脚*/
	*((uchar *)RxBuf[0]) |= (uchar)(BIT7 + BIT6 + BIT5);
	*((uchar *)RxBuf[1]) |= (uchar)BIT7;
	*((uchar *)RxBuf[2]) |= (uchar)BIT0;
	*((uchar *)RxBuf[3]) |= (uchar)(BIT7 + BIT6 + BIT5);
	*((uchar *)RxBuf[4]) |= (uchar)BIT7;
	*((uchar *)RxBuf[5]) |= (uchar)BIT0;

	FlashErase(ADR,WORD,LEN);//擦除写入Flash中的数据

	__bis_SR_register(LPM3_bits);//进入低功耗状态
}

/*************************** 写字节  ******************************/

void FlashWriteByte(uint address,uchar *byte,uint count)
{
	FCTL3 = FWKEY;			//Flash解锁
	while(FCTL3 & BUSY);	//等待忙
	FCTL1 = FWKEY + WRT;	//选择写入模式
	while(count--)
	{
		*((uchar *)address) = *(byte++); //把单字节缓冲区的数据写入对应的地址
		address = address + 2;//写入的数据地址按顺序累加
	}
	FCTL1 = FWKEY;			//清除写入模式
	FCTL3 = FWKEY + LOCK;	//Flash上锁
	while(FCTL3 & BUSY);	//等待忙
}

/*************************** 写字  ******************************/

void FlashWriteWord(uint address,uint *word,uint count)
{
	FCTL3 = FWKEY;
	while(FCTL3 & BUSY);
	FCTL1 = FWKEY + WRT;
	while(count--)
	{
		*((uint *)address) = *(word++);//把双字节缓冲区的数据写入对应的地址
		address = address + 2;
	}
	FCTL1 = FWKEY;
	FCTL3 = FWKEY + LOCK;
	while(FCTL3 & BUSY);
}

/*************************** 读FLASH ***************************/

void FlashRead(uint address,uint *buf,uint mode,uint length)
{
	if(mode)//判断读取模式：0 表示 单字节 ； 1 表示双字节。
	{
		while(length--)//判断读取数据的个数
		{
			*(buf++) = *((uint *)address);//把读取的双字节数据送入数据读取缓冲区
			address = address + 2;
		}
	}
	else
	{
		while(length--)
		{
			*((uchar *)buf++) = *((uchar *)address);//把读取的单字节数据送入数据读取缓冲区
			address = address + 2;
		}
	}

}

/*************************** 擦除FLASH ***************************/

void FlashErase(uint address,uint mode,uint length)
{
	FCTL3 = FWKEY;
	while(FCTL3 & BUSY);
	FCTL1 = FWKEY + ERASE;
	if(mode)//判断数据模式
	{
		while(length--)//判断擦除数据个数
		{
			*((uint *)address) = DELETE;//将对应地址上的双字节数据擦除
			address = address + 2;
		}
	}
	else
	{
		while(length--)
		{
			*((uchar *)address) = DELETE;//将对应地址上的单字节数据擦除
			address = address + 2;
		}
	}
	FCTL1 = FWKEY;
	FCTL3 = FWKEY + LOCK;
	while(FCTL3 & BUSY);
}
