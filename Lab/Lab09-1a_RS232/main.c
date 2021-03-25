/*
 * main.c
 */

#include <msp430f6638.h>
#include <stdint.h>
#include <stdio.h>
#include "dr_tft.h"

unsigned char flag0=0,flag1=0;
unsigned char send_data[]={'0','\0'};
unsigned char recv_data[]={'0','\0'};


void UART_RS232_Init(void);	//RS232接口初始化
void TimerA_Init(void);		//定时器TA初始化函数

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;//关闭看门狗

	UART_RS232_Init();		//初始化RS232接口
	initTFT();				//初始化TFT屏幕
	etft_AreaSet(0,0,319,239,0);	//TFT清屏
	TimerA_Init();					//初始化定时器
	_EINT();				//开启中断

	etft_DisplayString("Send Data: ",80,100,65535,0);	//TFT屏显示发送数据的标识
	etft_DisplayString("Recv Data: ",80,140,65535,0);	//TFT屏显示接收数据的标识

	while(1)
	{
		if(flag0)			//flag0在定时器计时到达1s时被赋值为1
		{
			flag0=0;
			UCA1TXBUF=send_data[0];		//写一个字符到发送缓存发送数据
			etft_DisplayString(send_data,170,100,65535,0);	//TFT屏上显示发送的字符
			send_data[0]++;				//字符加1
			if(send_data[0]>'9')		//字符超过'9'则重新置'0'
				send_data[0]='0';
		}

		if(flag1)			//flag1在接收中断中被赋值为1
		{
			flag1=0;
			etft_DisplayString(recv_data,170,140,65535,0);	//TFT屏幕上显示接收到的字符
		}
	}
}


#pragma vector=USCI_A1_VECTOR	//USCI中断服务函数
__interrupt void USCI_A1_ISR(void)
{
	switch(__even_in_range(UCA1IV,4))
	{
	case 0:break;			//无中断
	case 2:					//接收中断处理
		while(!(UCA1IFG&UCTXIFG));	//等待完成接收
		recv_data[0]=UCA1RXBUF;		//数据读出
		flag1=1;					//置标识数据的值
		break;
	case 4:break;			//发送中断不处理
	default:break;			//其他情况无操作

	}
}

#pragma vector = TIMER0_A0_VECTOR	//定时器TA中断服务函数
__interrupt void Timer_A (void)
{
	static unsigned char i=0;
	i++;
	if(i>=20)				//记满二十次为1s
	{
		i=0;
		flag0=1;			//改变标识数据的值
	}
}


void UART_RS232_Init(void)	//RS232接口初始化函数
{
	/*通过对P3.4。P3.5，P4.4，P4.5的配置实现通道选择
	 	 使USCI切换到UART模式*/
	P3DIR|=(1<<4)|(1<<5);
	P4DIR|=(1<<4)|(1<<5);
	P4OUT|=(1<<4);
	P4OUT&=~(1<<5);
	P3OUT|=(1<<5);
	P3OUT&=~(1<<4);
	P8SEL|=0x0c;	//模块功能接口设置，即P8.2与P8.3作为USCI的接收口与发射口
	UCA1CTL1|=UCSWRST;	//复位USCI
	UCA1CTL1|=UCSSEL_1;	//设置辅助时钟，用于发生特定波特率
	UCA1BR0=0x03;		//设置波特率
	UCA1BR1=0x00;
	UCA1MCTL=UCBRS_3+UCBRF_0;
	UCA1CTL1&=~UCSWRST;	//结束复位
	UCA1IE|=UCRXIE;		//使能接收中断
}

void TimerA_Init(void)		//定时器TA初始化函数
{
	TA0CTL |= MC_1 + TASSEL_2 + TACLR;	//时钟为SMCLK,比较模式，开始时清零计数器
	TA0CCTL0 = CCIE;					//比较器中断使能
	TA0CCR0  = 50000;					//比较值设为50000，相当于50ms的时间间隔
}
