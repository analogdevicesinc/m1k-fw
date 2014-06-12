#include <asf.h>
#include "conf_usb.h"

static bool main_b_vendor_enable = false;

uint8_t serial_number[USB_DEVICE_GET_SERIAL_NAME_LENGTH];

#define  MAIN_LOOPBACK_SIZE    1024
static uint8_t main_buf_loopback[MAIN_LOOPBACK_SIZE];

void main_vendor_bulk_in_received(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep);
void main_vendor_bulk_out_received(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep);

void init_build_usb_serial_number(void) {
	uint32_t uid[4];
	flash_read_unique_id(uid, 4);
	for (uint8_t i = 0; i < 16; i++) {
		serial_number[i*2] = "0123456789ABCDEF"[((uint8_t *)uid)[i]&0x0F];
		serial_number[i*2+1] = "0123456789ABCDEF"[((uint8_t *)uid)[i]&0xF0 >> 4];
	}
	serial_number[32] = 0;
}

int main(void)
{
	irq_initialize_vectors();
	cpu_irq_enable();
	init_build_usb_serial_number();
	// Initialize the sleep manager
	sleepmgr_init();
	sysclk_init();
	//board_init();

	// Start USB stack to authorize VBus monitoring
	udc_start();

	// The main loop manages only the power mode
	// because the USB management is done by interrupt
	while (true) {
		sleepmgr_enter_sleep();
	}
}

void main_suspend_action(void)
{
}

void main_resume_action(void)
{
}

void main_sof_action(void)
{
	if (!main_b_vendor_enable)
		return;
	//udd_get_frame_number()
}

bool main_vendor_enable(void)
{
	main_b_vendor_enable = true;
	// Start data reception on OUT endpoints
	main_vendor_bulk_in_received(UDD_EP_TRANSFER_OK, 0, 0);
	return true;
}

void main_vendor_disable(void)
{
	main_b_vendor_enable = false;
}

bool main_setup_out_received(void)
{
	udd_g_ctrlreq.payload = main_buf_loopback;
	udd_g_ctrlreq.payload_size = min(
			udd_g_ctrlreq.req.wLength,
			sizeof(main_buf_loopback));
	return true;
}

bool main_setup_in_received(void)
{
	udd_g_ctrlreq.payload = main_buf_loopback;
	udd_g_ctrlreq.payload_size =
			min( udd_g_ctrlreq.req.wLength,
			sizeof(main_buf_loopback) );
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

