/*
 * main.c
 */

#include<msp430f6638.h>

void main(void)
{
	 WDTCTL = WDTPW + WDTHOLD; //关闭看门狗
	 P7DIR |= BIT6;//设置P7.6口为输出口
	 P7SEL |= BIT6;//使能P7.6口第二功能位
	 DAC12_0CTL0 |= DAC12IR; //设置参考电压满刻度值，使Vout = Vref×(DAC12_xDAT/4096)
	 DAC12_0CTL0 |= DAC12SREF_1; //设置参考电压为AVCC
	 DAC12_0CTL0 |= DAC12AMP_5;	//设置运算放大器输入输出缓冲器为中速中电流
	 DAC12_0CTL0 |= DAC12CALON; //启动校验功能
	 DAC12_0CTL0 |= DAC12OPS;//选择第二通道P7.6
	 DAC12_0CTL0 |= DAC12ENC; //转化使能
	 DAC12_0DAT = 0xFFF; //输入数据

	__bis_SR_register(LPM4_bits); //进入低功耗状态
}
