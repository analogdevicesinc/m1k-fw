#include <asf.h>
#include "conf_usb.h"

static bool main_b_vendor_enable = false;
#define STRINGIFY(x)            #x

bool reset = false;
uint8_t serial_number[USB_DEVICE_GET_SERIAL_NAME_LENGTH];
const char hwversion[] = STRINGIFY(HW_VERSION);
const char fwversion[] = STRINGIFY(FW_VERSION);
const char gitversion[] = STRINGIFY(GIT_VERSION);

#define  MAIN_LOOPBACK_SIZE    512
static uint8_t main_buf_loopback[MAIN_LOOPBACK_SIZE];

void main_vendor_bulk_in_received(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep);
void main_vendor_bulk_out_received(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep);

void init_build_usb_serial_number(void) {
	uint32_t uid[4];
	flash_read_unique_id(uid, 4);
	for (uint8_t i = 0; i < 16; i++) {
		serial_number[i*2+1] = "0123456789ABCDEF"[((uint8_t *)uid)[i]&0x0F];
		serial_number[i*2] = "0123456789ABCDEF"[(((uint8_t *)uid)[i]&0xF0) >> 4];
	}
	serial_number[32] = 0;
}
int main(void)
{
	// enable LED
	pio_configure_pin(PIO_PA5_IDX, PIO_TYPE_PIO_OUTPUT_1 | PIO_DEFAULT);
	pio_configure_pin(PIO_PA3_IDX, PIO_TYPE_PIO_OUTPUT_1 | PIO_DEFAULT);
	pio_set_pin_low(PIO_PA3_IDX);
	
	irq_initialize_vectors();
	cpu_irq_enable();
	sysclk_init();
	// enable WDT for "fairly short"
	init_build_usb_serial_number();
	wdt_init(WDT, WDT_MR_WDRSTEN, 50, 50);
	// start USB
	udc_start();
	cpu_delay_us(100, F_CPU);
	udc_attach();
	while (true) {
		cpu_delay_us(10, F_CPU);
		if (!reset)
			wdt_restart(WDT);
	}
}

void main_suspend_action(void) { }

void main_resume_action(void) { }

void main_sof_action(void) {
	if (!main_b_vendor_enable)
		return;
	//udd_get_frame_number()
}

bool main_vendor_enable(void) {
	main_b_vendor_enable = true;
	main_vendor_bulk_in_received(UDD_EP_TRANSFER_OK, 0, 0);
	return true;
}

void main_vendor_disable(void) {
	main_b_vendor_enable = false;
}

bool main_setup_handle(void) {
	uint8_t* ptr = 0;
	uint16_t size = 1;
	if (Udd_setup_is_in()) {
	if (Udd_setup_type() == USB_REQ_TYPE_VENDOR) {
		switch (udd_g_ctrlreq.req.bRequest) {
			case 0x00: { // Info
				switch(udd_g_ctrlreq.req.wIndex){
					case 0:
						ptr = (uint8_t*)hwversion;
						size = sizeof(hwversion);
						break;
					case 1:
						ptr = (uint8_t*)fwversion;
						size = sizeof(fwversion);
						break;
					case 2:
						ptr = (uint8_t*)gitversion;
						size = sizeof(gitversion);
						break;
				}
				break;
			}
			case 0x01: {
				pio_toggle_pin(PIO_PA5_IDX);
				break;
			}
			case 0xBB: {
				flash_clear_gpnvm(1);
				reset = true;
				break;
			}
		}
	}
	}
	udd_g_ctrlreq.payload = ptr;
	udd_g_ctrlreq.payload_size = size;
	return true;
}

void main_vendor_bulk_in_received(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep)
{
	UNUSED(nb_transfered);
	UNUSED(ep);
	if (UDD_EP_TRANSFER_OK != status) {
		return; // Transfer aborted, then stop loopback
	}
	// Wait a full buffer
	udi_vendor_bulk_out_run(
			main_buf_loopback,
			sizeof(main_buf_loopback),
			main_vendor_bulk_out_received);
}

void main_vendor_bulk_out_received(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep)
{
	UNUSED(ep);
	if (UDD_EP_TRANSFER_OK != status) {
		return; // Transfer aborted, then stop loopback
	}
	// Send on IN endpoint the data received on endpoint OUT
	udi_vendor_bulk_in_run(
			main_buf_loopback,
			nb_transfered,
			main_vendor_bulk_in_received);
}

