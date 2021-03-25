#include <msp430.h>
#include "fw_queue.h"
#include "dr_usb.h"
#include "dr_tft.h"
#include <stdint.h>
#include "dr_cap_touch.h"

#define MCLK_FREQ  16000000
#define SMCLK_FREQ 16000000

#define BUFFER_LENGTH 256
uint8_t buffer_array1[BUFFER_LENGTH];
uint8_t buffer_array2[BUFFER_LENGTH];
uint8_t numb=0;
Queue buf_usb_to_uart, buf_uart_to_usb;

typedef struct
{
  const volatile uint8_t* PxIN;
  volatile uint8_t* PxOUT;
  volatile uint8_t* PxDIR;
  volatile uint8_t* PxREN;
  volatile uint8_t* PxSEL;
} GPIO_TypeDef;

const GPIO_TypeDef GPIO4 =
{
  &P4IN, &P4OUT, &P4DIR, &P4REN, &P4SEL
};

const GPIO_TypeDef GPIO5 =
{
  &P5IN, &P5OUT, &P5DIR, &P5REN, &P5SEL
};

const GPIO_TypeDef GPIO8 =
{
  &P8IN, &P8OUT, &P8DIR, &P8REN, &P8SEL
};

const GPIO_TypeDef* LED_GPIO[5] = {&GPIO8, &GPIO5, &GPIO4, &GPIO4, &GPIO4};
const uint8_t LED_PORT[5] = {BIT0, BIT7, BIT7, BIT6, BIT5};

void initClock()
{
  while(BAKCTL & LOCKIO) // Unlock XT1 pins for operation
    BAKCTL &= ~(LOCKIO);
  UCSCTL6 &= ~XT1OFF; //����XT1
  P7SEL |= BIT2 + BIT3; //XT2���Ź���ѡ��
  UCSCTL6 &= ~XT2OFF; //����XT2
  while (SFRIFG1 & OFIFG) { //�ȴ�XT1��XT2��DCO�ȶ�
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG); 
    SFRIFG1 &= ~OFIFG; 
  }
  
  UCSCTL4 = SELA__XT1CLK + SELS__XT2CLK + SELM__XT2CLK; //����DCO�������ܷ�
  
  UCSCTL1 = DCORSEL_5; //6000kHz~23.7MHz
  UCSCTL2 = MCLK_FREQ / (4000000 / 16); //XT2Ƶ�ʽϸߣ���Ƶ����Ϊ��׼�ɻ�ø��ߵľ���
  UCSCTL3 = SELREF__XT2CLK + FLLREFDIV__16; //XT2����16��Ƶ����Ϊ��׼
  while (SFRIFG1 & OFIFG) { //�ȴ�XT1��XT2��DCO�ȶ�
    UCSCTL7 &= ~(DCOFFG+XT1LFOFFG+XT2OFFG); 
    SFRIFG1 &= ~OFIFG; 
  }
  UCSCTL5 = DIVA__1 + DIVS__1 + DIVM__1; //�趨����CLK�ķ�Ƶ
  UCSCTL4 = SELA__XT1CLK + SELS__DCOCLK + SELM__DCOCLK; //�趨����CLK��ʱ��Դ
}

void initUART()
{
  //�趨��·ѡ������ѡ��RS232
  P3DIR |= BIT4;
  P3OUT &= ~BIT4;
  //����IO
  P8SEL |= BIT2 + BIT3;
  //���ô���ģ��
  UCA1CTL1 |= UCSWRST;
  UCA1CTL0 = 0;
  UCA1CTL1 = UCSWRST + UCSSEL__SMCLK + UCRXEIE;
  UCA1BRW = SMCLK_FREQ / 9600; //Ĭ�ϲ�����
  UCA1CTL1 &= ~UCSWRST;
}

void setBaudRate(uint32_t lBaudrate) // ��dr_usb/usbEventHandling.c����
{
  UCA1CTL1 |= UCSWRST;
  UCA1BRW = SMCLK_FREQ / lBaudrate;
  UCA1CTL1 &= ~UCSWRST;
}

void initButtons()
{
  P4REN |= 0x1F; //ʹ�ܰ����˿��ϵ�����������
  P4OUT |= 0x1F; //����״̬ 
}

void processUsbToUart()
{
  int i;
  static char display[64];

  if(UCA1IFG & UCRXIFG) //UART�յ��ַ�
  {
    if(queue_CanWrite(&buf_uart_to_usb)) //UART��USB���򻺳����п�λ
    {
      uint8_t temp = UCA1RXBUF;
      queue_Write(&buf_uart_to_usb, &temp, 1);
      display[numb]=temp;
      numb++;
      if(numb>63)
      {
    	  numb=0;
      }
      display[numb]='\0';
      etft_AreaSet(0,0,319,239,0);
      etft_DisplayString(display, 0, 0, 0xFFFF, 0);
    }
  }
  if(queue_CanRead(&buf_usb_to_uart)) //UART���ַ�����
  {
    if(UCA1IFG & UCTXIFG) //���ͻ���������
    {
      uint8_t temp;
      queue_Read(&buf_usb_to_uart, &temp, 1);
      UCA1TXBUF = temp;
    }
  }
  
  
  if(USB_GetConnectionStatus() == ST_ENUM_ACTIVE)
  {
    if(USB_ChannelCanRead(0)) //usb�����ݿɶ�
    {
      static uint8_t buf[64];
      int size = queue_CanWrite(&buf_usb_to_uart); //��ȡ������ʣ�೤��
      size = USB_ReadChannel(0, (char*)buf, size); //������ʣ�೤�ȵ�����
      buf[size]='\0';
      etft_AreaSet(0,0,319,239,0);
      etft_DisplayString((char*)buf, 0, 0, 0xFFFF, 0);
      queue_Write(&buf_usb_to_uart, buf, size); //д�뻺����
    }
  }
  if(USB_ChannelCanWrite(0))
  {
    static uint8_t buf[64];
    int size = queue_Read(&buf_uart_to_usb, buf, 64);
    if(size > 0)
    {
      USB_WriteChannel(0, (char*)buf, size);
    }
  }
}

void processButtons()
{
  char temp = ~P4IN; //���µ͵�ƽ
  if(!temp)
    return;
  if(CapTouch_ReadChannel(5) < 250) //�������ݰ���δ����
  {
    if(temp & BIT3) //S4
      USB_SendMouseOperation(0, 0, -1, 0);
    else if(temp & BIT1) //S6
      USB_SendMouseOperation(0, 0, 1, 0);
    else if(temp & BIT2) //S5
      USB_SendMouseOperation(1, 0, 0, 0);
    else if(temp & BIT0) //S7
      USB_SendMouseOperation(0, -1, 0, 0);
    else if(temp & BIT4) //S3
      USB_SendMouseOperation(0, 1, 0, 0);
    USB_SendMouseOperation(0, 0, 0, 0);
  }
  else
  {
    if(temp & BIT3) //S4
      USB_SendKeyboardKey(0, KEY_CODE_W);
    else if(temp & BIT1) //S6
      USB_SendKeyboardKey(0, KEY_CODE_S);
    else if(temp & BIT2) //S5
      USB_SendKeyboardKey(0, KEY_CODE_Enter);
    else if(temp & BIT0) //S7
      USB_SendKeyboardKey(0, KEY_CODE_A);
    else if(temp & BIT4) //S3
      USB_SendKeyboardKey(0, KEY_CODE_D);
    USB_SendKeyboardKey(0, KEY_CODE_None); //���ⱻ��Ϊ����������
  }
}

int main( void )
{
  WDTCTL = WDTPW + WDTHOLD;
  _DINT();
  initClock();  //��ʼ��ʱ��
  initQueue(&buf_usb_to_uart, buffer_array1, sizeof(buffer_array1)); //��ʼ��������
  initQueue(&buf_uart_to_usb, buffer_array2, sizeof(buffer_array2)); //��ʼ��������
  initUSB();   //��ʼ��USBģ��
  initUART();  //��ʼ���첽����ģ��
  initTFT();   //��ʼ��TFT
  _EINT();
  int i;
  for(i=0;i<5;++i)
    *LED_GPIO[i]->PxDIR |= LED_PORT[i]; //���ø�LED�����ڶ˿�Ϊ�������
  etft_AreaSet(0,0,319,239,0);
  while(1)
  {
    processUsbToUart();
    __delay_cycles(MCLK_FREQ / 1000); //��ʱ1ms������USB����
  }
}
