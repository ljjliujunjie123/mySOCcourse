/*
 * main.c
 */
#include <msp430f6638.h>
void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;                 //�رտ��Ź�

  //ʹ��XT2
  P7SEL |= BIT2+BIT3;                       //�˿�XT2��ѡ��
  UCSCTL6 &= ~(XT2OFF);                     //��XT2����
  UCSCTL6 |= XCAP_3;

  do							//ѭ���ȴ�XT1��DOC�����ȶ�
  {
    UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
                                            // ���XT1��XT2��DOC���ϱ�־λ
    SFRIFG1 &= ~OFIFG;                      // ���OSC���ϱ�־λ
  }while (SFRIFG1&OFIFG);                   // �ȴ������ȶ�

  UCSCTL6 &= ~(XT1DRIVE_3);                 // ����������

  //����δʹ�õ�����
  P1OUT = 0x00;
  P2OUT = 0x00;
  P3OUT = 0x00;
  P4OUT = 0x00;
  P5OUT = 0x00;
  P6OUT = 0x00;
  P8OUT = 0x00;
  P1DIR = 0xFF;
  P2DIR = 0xFF;
  P3DIR = 0xFF;
  P4DIR = 0xFF;
  P5DIR = 0xFF;
  P6DIR = 0xFF;
  P8DIR = 0xFF;

  USBKEYPID = 0x9628;            //��KEYandPIDΪ0x9628���������USB���üĴ�����
  USBPWRCTL &= ~(SLDOEN+VUSBEN); //����VUSB��LDO��SLDO
  USBKEYPID = 0x9600;            //��KEYandPIDΪ0x9600����ֹ����USB���üĴ�����

  //����SVS
  PMMCTL0_H = PMMPW_H;                //PMM����
  SVSMHCTL &= ~(SVMHE+SVSHE);         //���ø�ѹ��SVS
  SVSMLCTL &= ~(SVMLE+SVSLE);         //���õ�ѹ��SVS

  __bis_SR_register(LPM1_bits);   	  //����͹���ģʽ1
  __no_operation();                   //��ʱһ����������
}

