/*
 * dr_lcdseg.h
 *
 *  Created on: 2013-5-28
 *  Additional: DengPihui
 */

#ifndef DR_LCDSEG_H_
#define DR_LCDSEG_H_

void initLcdSeg();	//显示频初始化
void LCDSEG_SetDigit(int pos, int value);
/* 在pos(0<=pos<=5)位置写入一个
 * 整数value（-1《=value《=16，0《=pos《=5），
 * value=16表示写入负号，value=-1表示清除该位
 */
void LCDSEG_SetSpecSymbol(int pos);//在pos位置清除小数点（3<=pos<=5）
void LCDSEG_ResetSpecSymbol(int pos);//在pos位置写小数点（3<=pos<=5）
void LCDSEG_DisplayNumber(long num, int dppos);//让段式液晶显示num这个整数，并在dppos位置处添加小数点(《0《dppos《=3)


#endif /* DR_LCDSEG_H_ */
