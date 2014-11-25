#ifndef _CONF_BOARD_H_
#define _CONF_BOARD_H_

#include <asf.h>

#define CONF_BOARD_USB_PORT

#define N_SYNC (1<<16)
#define N_LDAC (1<<30)
#define CNV (1<<31)
#define N_CLR (1<<15)
#define IO0 (1<<0)
#define IO1 (1<<1)
#define IO2 (1<<2)
#define IO3 (1<<3)
#define PWR (1<<17)

#define LED_B (1 << 28)
#define LED_G (1 << 29)
#define LED_R (1 << 15)

#endif
