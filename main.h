#ifndef _MAIN_H_
#define _MAIN_H_

bool main_vendor_enable(void);

void main_vendor_disable(void);

void main_sof_action(void);

void main_suspend_action(void);

void main_resume_action(void);

bool main_setup_handle(void);

void init_build_usb_serial_number(void);

void hardware_init(void);

void setup_dac(void);
void write_dac(uint8_t cmd, uint8_t addr, uint16_t val);
void set_pots(uint8_t ch, uint8_t r1, uint8_t r2);

void main_vendor_bulk_out_received(udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep);
void main_vendor_bulk_in_received(udd_ep_status_t status, iram_size_t nb_transfered, udd_ep_id_t ep);


#endif // _MAIN_H_
