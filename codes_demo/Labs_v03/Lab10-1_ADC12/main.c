/*
 * main.c
 */

#include<msp430f6638.h>

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;//�رտ��Ź�
	P4DIR |= BIT5 + BIT6 + BIT7;//����GPIO����
	P5DIR |= BIT7;
	P8DIR |= BIT0;
	ADC12CTL0 |= ADC12MSC;//�Զ�ѭ������ת��
	ADC12CTL0 |= ADC12ON;//����ADC12ģ��
	ADC12CTL1 |= ADC12CONSEQ1 ;//ѡ��ͨ��ѭ������ת��
	ADC12CTL1 |= ADC12SHP;//��������ģʽ
	ADC12MCTL0 |= ADC12INCH_15; //ѡ��ͨ��15�����Ӳ����λ��
	ADC12CTL0 |= ADC12ENC;
	volatile unsigned int value = 0;//�����жϱ���
	while(1)
	{
		ADC12CTL0 |= ADC12SC;//��ʼ����ת��
		value = ADC12MEM0;//�ѽ����������
		if(value > 5)//�жϽ����Χ
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
