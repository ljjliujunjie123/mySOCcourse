#include <msp430.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fw_public.h"
#include "TFT\dr_tft.h"
#include "key_led\dr_buttons_and_leds.h"
#include "dr_i2c.h"
#include "app_batt.h"
#include "app_bmp.h"
#include "app_wav_player.h"
#include "app_kb_and_ls.h"
#include "dr_usbmsc.h"
#include "dr_audio_io.h"
#include "dr_dc_motor.h"
#include "dr_step_motor.h"
#include "dr_lcdseg.h"
#include "dr_sdcard.h"

#define MCLK_FREQ   20000000
#define SMCLK_FREQ  20000000
#define XT2CLK_FREQ  4000000
#define REFCLK_FREQ     1000
#define captouch_max 200
