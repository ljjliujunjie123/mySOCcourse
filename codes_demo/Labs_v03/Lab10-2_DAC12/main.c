/*
 * main.c
 */

#include<msp430f6638.h>

void main(void)
{
	 WDTCTL = WDTPW + WDTHOLD; //�رտ��Ź�
	 P7DIR |= BIT6;//����P7.6��Ϊ�����
	 P7SEL |= BIT6;//ʹ��P7.6�ڵڶ�����λ
	 DAC12_0CTL0 |= DAC12IR; //���òο���ѹ���̶�ֵ��ʹVout = Vref��(DAC12_xDAT/4096)
	 DAC12_0CTL0 |= DAC12SREF_1; //���òο���ѹΪAVCC
	 DAC12_0CTL0 |= DAC12AMP_5;	//��������Ŵ����������������Ϊ�����е���
	 DAC12_0CTL0 |= DAC12CALON; //����У�鹦��
	 DAC12_0CTL0 |= DAC12OPS;//ѡ��ڶ�ͨ��P7.6
	 DAC12_0CTL0 |= DAC12ENC; //ת��ʹ��
	 DAC12_0DAT = 0xFFF; //��������

	__bis_SR_register(LPM4_bits); //����͹���״̬
}
