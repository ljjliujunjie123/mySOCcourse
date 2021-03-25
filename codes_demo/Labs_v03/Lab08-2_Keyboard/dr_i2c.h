/*
 * dr_i2c.h
 *
 *  Created on: 2013-6-6
 *      Author: DengPihui
 */

#ifndef DR_I2C_H_
#define DR_I2C_H_

#include <stdint.h>

#ifndef SMCLK_FREQ
  #define SMCLK_FREQ 4000000
#endif

#define I2C_FREQ 100000

//初始化I2C总线
void initI2C();

//请求向reg写入value，返回数据是否进入缓冲区
int I2C_RequestSend(uint8_t slave, uint8_t reg, uint8_t value);

//添加一个I2C读请求，返回查询用代号
int I2C_AddRegQuery(uint8_t slave, uint8_t reg);

//检查一个I2C读请求是否已读到新值
int I2C_QueryHasNew(int index);

//要求返回一个I2C读请求的值，同时标记原始值为旧，准备进行下一次查询
uint16_t I2C_CheckQuery(int index);


#endif /* DR_I2C_H_ */
