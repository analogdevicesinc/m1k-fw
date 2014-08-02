#ifndef _MAIN_H_
#define _MAIN_H_

typedef struct IN_packet{
	uint16_t data_a_v[256];
	uint16_t data_a_i[256];
	uint16_t data_b_v[256];
	uint16_t data_b_i[256];
} IN_packet;

typedef struct OUT_packet{
	uint32_t data_a[256];
	uint32_t data_b[256];
} OUT_packet;

bool main_vendor_enable(void);

void main_vendor_disable(void);

void main_sof_action(void);

void main_suspend_action(void);

void main_resume_action(void);

bool main_setup_handle(void);

void init_build_usb_serial_number(void);

void hardware_init(void);

void write_pots(uint8_t ch, uint8_t r1, uint8_t r2);

void main_vendor_bulk_out_received(udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep);
void main_vendor_bulk_in_received(udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep);

#endif
