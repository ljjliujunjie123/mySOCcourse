#ifndef __DR_LCDSEG_H_
#define __DR_LCDSEG_H_

void initLcdSeg();
void LCDSEG_SetDigit(int pos, int value);
void LCDSEG_SetSpecSymbol(int pos);
void LCDSEG_ResetSpecSymbol(int pos);
void LCDSEG_DisplayNumber(int32_t num, int dppos);

#endif
