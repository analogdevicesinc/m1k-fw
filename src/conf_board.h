#ifndef _CONF_BOARD_H_
#define _CONF_BOARD_H_

#define BOARD_FREQ_SLCK_XTAL		(32768U)
#define BOARD_FREQ_SLCK_BYPASS		(32768U)
#define BOARD_FREQ_MAINCK_XTAL		(12000000U)
#define BOARD_FREQ_MAINCK_BYPASS	(12000000U)
#define BOARD_MCK					CHIP_FREQ_CPU_MAX
#define BOARD_OSC_STARTUP_US		15625

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

#endif
