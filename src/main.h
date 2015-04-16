#ifndef _MAIN_H_
#define _MAIN_H_

#include <asf.h>

#define stringify(x)			#x
#define xstringify(s) stringify(s)
#define SWAP16(x)        ((((x) & 0xff00)>> 8) | (((x) & 0x00ff) << 8))
#define A 0
#define B 1

#define cal_table_base = 0x00080000 + 256*254; 

typedef enum chan_mode{
	DISABLED = 0,
	SVMI = 1,
	SIMV = 2,
} chan_mode;


typedef union IN_packet {
	struct {
		uint16_t data_a_v[256];
		uint16_t data_a_i[256];
		uint16_t data_b_v[256];
		uint16_t data_b_i[256];
	};
	uint16_t data[1024];
} IN_packet;

typedef union OUT_packet {
	struct {
		uint16_t data_a[256];
		uint16_t data_b[256];
	};
	uint16_t data[512];
} OUT_packet;

typedef struct rgb {
    uint8_t r;
	uint8_t g;
	uint8_t b;
} rgb;

IN_packet packets_in[2];
OUT_packet packets_out[2];

typedef enum ch_params {
	i0_dac = 0,
	v0_adc = 1,
	i0_adc = 2,
	p1_simv = 3,
	p2_simv = 4
} ch_params;

uint16_t cal_data[IFLASH0_PAGE_SIZE/sizeof(uint16_t)];

uint8_t serial_number[USB_DEVICE_GET_SERIAL_NAME_LENGTH];
volatile uint32_t slot_offset;
volatile uint32_t packet_index_in;
volatile uint32_t packet_index_out;
volatile uint32_t packet_index_send_in;
volatile uint32_t packet_index_send_out;
volatile uint16_t start_frame;
volatile bool send_in;
volatile bool send_out;
volatile bool sending_in;
volatile bool sending_out;
volatile bool sent_in;
volatile bool sent_out;
volatile bool current_chan;
volatile bool reset;
volatile bool main_b_vendor_enable;
volatile bool start_timer;

uint8_t ret_data[64];

bool main_vendor_enable(void);

void main_vendor_disable(void);

void main_sof_action(void);

void main_suspend_action(void);

void h_to_rgb(uint8_t , rgb * c);

void main_resume_action(void);

bool main_setup_handle(void);
bool msft_string_handle(void);

void init_build_usb_serial_number(void);

void init_hardware(void);
void config_hardware(void);

void write_ad5122(uint32_t ch, uint8_t r1, uint8_t r2);
void write_adm1177(uint8_t v);
void write_ad5663(uint8_t conf, uint16_t data);
void read_adm1177(uint8_t b[], uint8_t c);
void set_mode(uint32_t chan, chan_mode m);

void main_vendor_bulk_out_received(udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep);
void main_vendor_bulk_in_received(udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep);

#endif
