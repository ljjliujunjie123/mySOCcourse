/*
 * main.c
 */

#include<msp430f6638.h>

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;//关闭看门狗
	P4DIR |= BIT5 + BIT6 + BIT7;//配置GPIO引脚
	P5DIR |= BIT7;
	P8DIR |= BIT0;
	ADC12CTL0 |= ADC12MSC;//自动循环采样转换
	ADC12CTL0 |= ADC12ON;//启动ADC12模块
	ADC12CTL1 |= ADC12CONSEQ1 ;//选择单通道循环采样转换
	ADC12CTL1 |= ADC12SHP;//采样保持模式
	ADC12MCTL0 |= ADC12INCH_15; //选择通道15，连接拨码电位器
	ADC12CTL0 |= ADC12ENC;
	volatile unsigned int value = 0;//设置判断变量
	while(1)
	{
		ADC12CTL0 |= ADC12SC;//开始采样转换
		value = ADC12MEM0;//把结果赋给变量
		if(value > 5)//判断结果范围
			P4OUT |= BIT5;
		else
			P4OUT &= ~BIT5;
		if(value >= 800)
			P4OUT |= BIT6;
		else
			P4OUT &= ~BIT6;
		if(value >= 1600)
			P4OUT |= BIT7;
		else
			P4OUT &= ~BIT7;
		if(value >= 2400)
			P5OUT |= BIT7;
		else
			P5OUT &= ~BIT7;
		if(value >= 3200)
			P8OUT |= BIT0;
		else
			P8OUT &= ~BIT0;
	}
}
