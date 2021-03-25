#include <msp430.h>
#include <stdint.h>
#include "dr_i2c.h"
#include "fw_public.h"

#define OVERFLOW_ADD(val, limit) \
{ \
  val++; \
  if(val >= (limit)) \
    val = 0; \
}

#define I2C_NONE -1
#define SS_WAIT_FOR_SEND_REG 0
#define SS_WAIT_FOR_SEND_DATA 1
#define SS_WAIT_FOR_END 2
#define RS_WAIT_FOR_SEND_REG 3
#define RS_WAIT_FOR_SEND_READ_START 4
#define RS_WAIT_FOR_RECV 5
#define RS_WAIT_FOR_RECV2 6
int curI2CStatus = I2C_NONE;

void initI2C()
{
  UCB0CTL1 |= UCSWRST; //复位UCB0
  
  UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC; //7位地址，单主机，本机为主机，I2C，同步
  UCB0CTL1 = UCSSEL__SMCLK + UCSWRST; //选择SMCLK作为时钟，保持复位
  UCB0BR0 = (SMCLK_FREQ / I2C_FREQ) & 0xFF;
  UCB0BR1 = ((SMCLK_FREQ / I2C_FREQ) >> 8) & 0xFF;
  
  P2SEL |= BIT1 + BIT2; //选择P2.1、P2.2的外设功能
  P2MAP1 = PM_UCB0SDA; //P2.1作为SDA
  P2MAP2 = PM_UCB0SCL; //P2.2作为SCL
  
  UCB0CTL1 &= ~UCSWRST; //清除复位
  
  UCB0IE |= UCNACKIE; //启用NACK中断
}

typedef struct
{
  uint8_t slave;
  uint8_t reg;
  uint8_t value;
} I2C_TXREQ;

#define TXREQ_BUFFER_SIZE 20
I2C_TXREQ tx_requests[TXREQ_BUFFER_SIZE];
int curSendingNo = 0, curTxEmptyNo = 0;

int I2C_Send()
{
  uint16_t gie;
  _ECRIT(gie);

  if(curI2CStatus != I2C_NONE)
  {
    _LCRIT(gie);
	return 0;
  }
  
  if(curSendingNo != curTxEmptyNo)
  {
    curI2CStatus = SS_WAIT_FOR_SEND_REG; //进入待发状态
    UCB0I2CSA = tx_requests[curSendingNo].slave; //设定从机地址
    UCB0CTL1 |= UCTR; //设置发送模式
    UCB0CTL1 |= UCTXSTT; //发送START
    UCB0IE |= UCTXIE; //启用发送中断
    UCB0IE &= ~UCRXIE; //关闭接收中断
    _LCRIT(gie);
    return 1;
  }
  _LCRIT(gie);
  return 0;
}

int I2C_RequestSend(uint8_t slave, uint8_t reg, uint8_t value)
{
  uint16_t gie;
  _ECRIT(gie);

  int temp = curTxEmptyNo;
  OVERFLOW_ADD(temp, TXREQ_BUFFER_SIZE);
  if(temp == curSendingNo)
  {
    _LCRIT(gie);
	return 0; //缓冲区接近溢出，不可写
  }
  
  tx_requests[curTxEmptyNo].slave = slave;
  tx_requests[curTxEmptyNo].reg = reg;
  tx_requests[curTxEmptyNo].value = value;
  
  curTxEmptyNo = temp;
  
  I2C_Send();
  
  _LCRIT(gie);
  return 1;
}

typedef struct
{
  uint8_t slave;
  uint8_t reg;
  uint16_t value;
  uint8_t hasnew;
} I2C_RXREG;

#define RXREG_BUFFER_SIZE 20
I2C_RXREG rx_regs[RXREG_BUFFER_SIZE];
int rxreg_count = 0;
int curRecvingNo = 0;

int I2C_StartQuery()
{
  if(curI2CStatus != I2C_NONE)
    return 0;
  if(rxreg_count == 0)
    return 0;
  
  int count = 0;  
  while(rx_regs[curRecvingNo].hasnew != 0)
  { //当前考虑读取的位置的数据还未取出
    OVERFLOW_ADD(curRecvingNo, rxreg_count); //测试下一个点
    count++;
    if(count >= rxreg_count) //已全部检查一遍，无需要更新的点
      return 0;
  }
  
  curI2CStatus = RS_WAIT_FOR_SEND_REG;
  UCB0I2CSA = rx_regs[curRecvingNo].slave; //设定从机地址
  UCB0CTL1 |= UCTR; //设置发送模式
  UCB0CTL1 |= UCTXSTT; //发送START
  UCB0IE |= UCTXIE; //启用发送中断
  UCB0IE &= ~UCRXIE; //关闭接收中断
  return 1;
}

int I2C_AddRegQuery(uint8_t slave, uint8_t reg)
{
  uint16_t gie;
  _ECRIT(gie);

  if(rxreg_count >= RXREG_BUFFER_SIZE)
  {
    _LCRIT(gie);
    return -1; //满
  }
  
  rx_regs[rxreg_count].slave = slave;
  rx_regs[rxreg_count].reg = reg;
  rx_regs[rxreg_count].value = 0;
  rx_regs[rxreg_count].hasnew = 0;
  rxreg_count++;
  
  I2C_StartQuery();
  
  _LCRIT(gie);
  return rxreg_count-1;
}

int I2C_QueryHasNew(int index)
{
  return rx_regs[index].hasnew;
}

uint16_t I2C_CheckQuery(int index)
{
  rx_regs[index].hasnew = 0;
  uint16_t gie;
  _ECRIT(gie);
  I2C_StartQuery(); //现在可以开始新查询
  _LCRIT(gie);
  return rx_regs[index].value;
}

#pragma vector=USCI_B0_VECTOR
__interrupt void i2cInterrupt()
{
  if(UCB0IFG & UCNACKIFG)
  {
    switch(curI2CStatus)
    {
    case SS_WAIT_FOR_SEND_REG:
    case SS_WAIT_FOR_SEND_DATA:
    case SS_WAIT_FOR_END:
      OVERFLOW_ADD(curSendingNo, TXREQ_BUFFER_SIZE);
      break;
    case RS_WAIT_FOR_SEND_REG:
    case RS_WAIT_FOR_SEND_READ_START:
    case RS_WAIT_FOR_RECV:
    case RS_WAIT_FOR_RECV2:
      OVERFLOW_ADD(curRecvingNo, rxreg_count);
      break;
    default:
      break;
    }
    curI2CStatus = I2C_NONE; //检查发送与接收均需要此状态
    UCB0IFG = 0;
    if(!I2C_Send()) 
      if(!I2C_StartQuery())
        UCB0CTL1 |= UCTXSTP;
  }
  else
  {
    switch(curI2CStatus)
    {
    case SS_WAIT_FOR_SEND_REG:
      UCB0TXBUF = tx_requests[curSendingNo].reg;
      curI2CStatus = SS_WAIT_FOR_SEND_DATA;
      break;
      
    case SS_WAIT_FOR_SEND_DATA:
      UCB0TXBUF = tx_requests[curSendingNo].value;
      curI2CStatus = SS_WAIT_FOR_END;
      break;
      
    case SS_WAIT_FOR_END:
      curI2CStatus = I2C_NONE; //检查发送与接收均需要此状态
      UCB0IE &= ~UCTXIE;
      OVERFLOW_ADD(curSendingNo, TXREQ_BUFFER_SIZE);
      
      UCB0CTL1 |= UCTXSTP;
      while(UCB0CTL1 & UCTXSTP); //等待发送STOP结束
      __delay_cycles(100); //延迟5us
      if(!I2C_Send()) 
        I2C_StartQuery();
      
      /*if(!I2C_Send()) 
        if(!I2C_StartQuery())
          UCB0CTL1 |= UCTXSTP;*/
      
      break;
      
    case RS_WAIT_FOR_SEND_REG:
      UCB0TXBUF = rx_regs[curRecvingNo].reg;
      curI2CStatus = RS_WAIT_FOR_SEND_READ_START;
      break;
      
    case RS_WAIT_FOR_SEND_READ_START:
      UCB0I2CSA = rx_regs[curRecvingNo].slave; //设定从机地址
      UCB0CTL1 &= ~UCTR; //设置接收模式
      UCB0CTL1 |= UCTXSTT; //发送START
      UCB0IE &= ~UCTXIE; //关闭发送中断
      UCB0IE |= UCRXIE; //启动接收中断
      curI2CStatus = RS_WAIT_FOR_RECV;
      break;
      
    case RS_WAIT_FOR_RECV:
      curI2CStatus = RS_WAIT_FOR_RECV2;
      rx_regs[curRecvingNo].value = UCB0RXBUF; //(UCB0RXBUF << 8) & 0xFF00;  
      UCB0CTL1 |= UCTXSTP; //告知将在下一字节后停止
      break;
      
    case RS_WAIT_FOR_RECV2:
      curI2CStatus = I2C_NONE; //检查发送与接收均需要此状态
      UCB0IE &= ~UCRXIE;
      
      rx_regs[curRecvingNo].value |= (UCB0RXBUF << 8) & 0xFF00;//UCB0RXBUF;
      rx_regs[curRecvingNo].hasnew = 1;
      
      OVERFLOW_ADD(curRecvingNo, rxreg_count); //继续查询需要下一地址
      
      while(UCB0CTL1 & UCTXSTP); //等待发送STOP结束
      __delay_cycles(100); //延迟5us
      if(!I2C_Send()) 
        I2C_StartQuery();
      
      break;
      
    default:
      break;
    }
  }
}
