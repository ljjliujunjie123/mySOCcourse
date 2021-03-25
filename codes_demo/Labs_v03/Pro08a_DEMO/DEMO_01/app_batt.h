#ifndef __APP_BATT_H_
#define __APP_BATT_H_

#define TEMP_ADDR 0x2A
#define TEMP_LOCAL 0x00
#define TEMP_REMOTE 0x01
#define TEMP_CONFIG1 0x09
#define TEMP_CONFIG2 0x0A
#define TEMP_NCORR   0x21

#define BATT_ADDR 0x55
#define BATT_VOLTAGE 0x04
#define BATT_CURRENT 0x10
#define BATT_SOC 0x1C
#define BATT_CAPA 0x0C
#define BATT_FLAG 0x06

void initBATT(void);

void BATTstatus(void);

void BATTmain(void);

#endif //__APP_BATT_H_
