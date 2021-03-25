/*
 * main.c
 */
#include <msp430f6638.h>

/*************************** �궨�� ******************************/

typedef unsigned int uint;
typedef unsigned char uchar;

#define LEN 6 //�궨�建������С
#define ADR 0x1880//�궨��д��Flash����ʼ��ַ��λ��Segment C
#define BYTE 0//���ֽ�д���־
#define WORD 1//˫�ֽ�д���־
#define DELETE 0//���ֵ

//�궨����Ӧ�Ĵ����ĵ�ַ
#define P4_OUT 0x0223
#define P4_DIR 0x0225
#define P5_OUT 0X0242
#define P5_DIR 0x0244
#define P8_OUT 0x0263
#define P8_DIR 0x0265

/************************** ��������  *****************************/

void FlashWriteByte(uint address,uchar *byte,uint count);
void FlashWriteWord(uint address,uint *word,uint count);
void FlashErase(uint address,uint mode,uint length);
void FlashRead(uint address,uint *buf,uint mode,uint length);

/*************************** ������  ******************************/

void main(void)
{
	WDTCTL = WDTPW + WDTHOLD;//�رտ��Ź�
	//uchar BSetBuf[LEN] = {0}; �����ֽ�д�뻺������������δ�õ�
	uint WSetBuf[LEN] = {P4_DIR,P5_DIR,P8_DIR,P4_OUT,P5_OUT,P8_OUT};
	// ˫�ֽ�д�뻺����
	uint RxBuf[LEN] = {0};//���ݶ�ȡ������
	FlashErase(ADR,WORD,LEN);//����д��Flash�е�����
	FlashWriteWord(ADR,WSetBuf,LEN);//˫�ֽ�д�뺯��
	FlashRead(ADR,RxBuf,WORD,LEN);//��ȡ����

	/*ȡ�û�������������Ϊ��ַ�����丳ֵ���˴����õ���LED�Ƶ�GPIO�ܽ�*/
	*((uchar *)RxBuf[0]) |= (uchar)(BIT7 + BIT6 + BIT5);
	*((uchar *)RxBuf[1]) |= (uchar)BIT7;
	*((uchar *)RxBuf[2]) |= (uchar)BIT0;
	*((uchar *)RxBuf[3]) |= (uchar)(BIT7 + BIT6 + BIT5);
	*((uchar *)RxBuf[4]) |= (uchar)BIT7;
	*((uchar *)RxBuf[5]) |= (uchar)BIT0;

	FlashErase(ADR,WORD,LEN);//����д��Flash�е�����

	__bis_SR_register(LPM3_bits);//����͹���״̬
}

/*************************** д�ֽ�  ******************************/

void FlashWriteByte(uint address,uchar *byte,uint count)
{
	FCTL3 = FWKEY;			//Flash����
	while(FCTL3 & BUSY);	//�ȴ�æ
	FCTL1 = FWKEY + WRT;	//ѡ��д��ģʽ
	while(count--)
	{
		*((uchar *)address) = *(byte++); //�ѵ��ֽڻ�����������д���Ӧ�ĵ�ַ
		address = address + 2;//д������ݵ�ַ��˳���ۼ�
	}
	FCTL1 = FWKEY;			//���д��ģʽ
	FCTL3 = FWKEY + LOCK;	//Flash����
	while(FCTL3 & BUSY);	//�ȴ�æ
}

/*************************** д��  ******************************/

void FlashWriteWord(uint address,uint *word,uint count)
{
	FCTL3 = FWKEY;
	while(FCTL3 & BUSY);
	FCTL1 = FWKEY + WRT;
	while(count--)
	{
		*((uint *)address) = *(word++);//��˫�ֽڻ�����������д���Ӧ�ĵ�ַ
		address = address + 2;
	}
	FCTL1 = FWKEY;
	FCTL3 = FWKEY + LOCK;
	while(FCTL3 & BUSY);
}

/*************************** ��FLASH ***************************/

void FlashRead(uint address,uint *buf,uint mode,uint length)
{
	if(mode)//�ж϶�ȡģʽ��0 ��ʾ ���ֽ� �� 1 ��ʾ˫�ֽڡ�
	{
		while(length--)//�ж϶�ȡ���ݵĸ���
		{
			*(buf++) = *((uint *)address);//�Ѷ�ȡ��˫�ֽ������������ݶ�ȡ������
			address = address + 2;
		}
	}
	else
	{
		while(length--)
		{
			*((uchar *)buf++) = *((uchar *)address);//�Ѷ�ȡ�ĵ��ֽ������������ݶ�ȡ������
			address = address + 2;
		}
	}

}

/*************************** ����FLASH ***************************/

void FlashErase(uint address,uint mode,uint length)
{
	FCTL3 = FWKEY;
	while(FCTL3 & BUSY);
	FCTL1 = FWKEY + ERASE;
	if(mode)//�ж�����ģʽ
	{
		while(length--)//�жϲ������ݸ���
		{
			*((uint *)address) = DELETE;//����Ӧ��ַ�ϵ�˫�ֽ����ݲ���
			address = address + 2;
		}
	}
	else
	{
		while(length--)
		{
			*((uchar *)address) = DELETE;//����Ӧ��ַ�ϵĵ��ֽ����ݲ���
			address = address + 2;
		}
	}
	FCTL1 = FWKEY;
	FCTL3 = FWKEY + LOCK;
	while(FCTL3 & BUSY);
}
