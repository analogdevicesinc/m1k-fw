#include <asf.h>
#include "conf_usb.h"

static bool main_b_vendor_enable = false;
#define STRINGIFY(x)            #x

bool reset = false;
uint8_t serial_number[USB_DEVICE_GET_SERIAL_NAME_LENGTH];
uint8_t ret_data[64];
const char hwversion[] = STRINGIFY(HW_VERSION);
const char fwversion[] = STRINGIFY(FW_VERSION);
const char gitversion[] = STRINGIFY(GIT_VERSION);

static  usart_spi_opt_t USART_SPI_ADC =
{
	.baudrate     = 1000000,
	.char_length   = 8,
	.spi_mode      = SPI_MODE_0,
	.channel_mode  = US_MR_CHMODE_NORMAL
};

static  usart_spi_opt_t USART_SPI_DAC =
{
	.baudrate     = 1000000,
	.char_length   = 8,
	.spi_mode      = SPI_MODE_1,
	.channel_mode  = US_MR_CHMODE_NORMAL
};

static twi_options_t TWIM_CONFIG =
{
	.master_clk = F_CPU, // main CPU speed
	.speed = 100000, // 100KHz -> normal speed i2c
	.chip = 0, // master
	.smbus = 0,
};



static uint8_t main_buf_loopback[512];

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

void hardware_init(void) {

// PWR
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB17, PIO_DEFAULT);

// SDA
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA9A_TWD0, PIO_DEFAULT);
// SCL
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA10A_TWCK0, PIO_DEFAULT);

// LDAC_N
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA14, PIO_DEFAULT);
// CLR_N
	pio_configure(PIOA, PIO_OUTPUT_1, PIO_PA15, PIO_DEFAULT);
// SYNC_N
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA16, PIO_DEFAULT);
// DAC_CLK
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA17A_SCK0, PIO_DEFAULT);
// DAC_MOSI
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA18A_TXD0, PIO_DEFAULT);

// CHA_ADC_MISO
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA21A_RXD1, PIO_DEFAULT);
// CHA_ADC_MOSI
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA20A_TXD1, PIO_DEFAULT);
// CHA_ADC_CLK
	pio_configure(PIOB, PIO_PERIPH_B, PIO_PA24B_SCK1, PIO_DEFAULT);

// ADC_CNV
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA26, PIO_DEFAULT);

// CHB_ADC_MISO
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA22A_TXD2, PIO_DEFAULT);
// CHB_ADC_MOSI
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA23A_RXD2, PIO_DEFAULT);
// CHB_ADC_CLK
	pio_configure(PIOB, PIO_PERIPH_B, PIO_PA25B_SCK2, PIO_DEFAULT);

// CHA_SWMODE
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB19, PIO_DEFAULT);
// CHB_SWMODE
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB20, PIO_DEFAULT);

// CHA_SAFE_SWITCH
	pio_configure(PIOB, PIO_INPUT, PIO_PB4, PIO_DEFAULT);
// CHB_SAFE_SWITCH
	pio_configure(PIOB, PIO_INPUT, PIO_PB18, PIO_DEFAULT);

// CHA_OUT_CONNECT
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB0, PIO_DEFAULT);
// CHA_OUT_50OPAR
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB1, PIO_DEFAULT);
// CHA_OUT_10KSER
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB2, PIO_DEFAULT);
// CHA_OUT_1MOPAR
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB3, PIO_DEFAULT);

// CHB_OUT_CONNECT
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB5, PIO_DEFAULT);
// CHB_OUT_50OPAR
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB6, PIO_DEFAULT);
// CHB_OUT_10KSER
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB7, PIO_DEFAULT);
// CHB_OUT_1MOPAR
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB8, PIO_DEFAULT);

// devboard LED
	//pio_configure(PIOA, PIO_OUTPUT_1, PIO_PA5, PIO_DEFAULT);
	//pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA3, PIO_DEFAULT);

// 1MHz SPI
	usart_init_spi_master(USART0, &USART_SPI_DAC, 1000000);
	usart_init_spi_master(USART1, &USART_SPI_ADC, 1000000);
	usart_init_spi_master(USART2, &USART_SPI_ADC, 1000000);

// 100khz I2C
	twi_reset(TWI0);
	twi_enable_master_mode(TWI0);
	twi_master_init(TWI0, &TWIM_CONFIG);

// enable peripheral clock access
	pmc_enable_periph_clk(ID_TWI0);
	pmc_enable_periph_clk(ID_USART0);
	pmc_enable_periph_clk(ID_USART1);
	pmc_enable_periph_clk(ID_USART2);
}

int main(void)
{
	irq_initialize_vectors();
	cpu_irq_enable();
	sysclk_init();
	// convert chip UID to ascii string of hex representation
	init_build_usb_serial_number();
	// enable WDT for "fairly short"
	wdt_init(WDT, WDT_MR_WDRSTEN, 50, 50);
	// setup peripherals
	hardware_init();
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
			case 0xEE: {
				Pio *p_pio = (Pio *)((uint32_t)PIOA + (PIO_DELTA * ((udd_g_ctrlreq.req.wValue&0xFF) >> 5)));
				ret_data[0] = (p_pio->PIO_ODSR & (1 << (udd_g_ctrlreq.req.wValue& 0x1F))) != 0;
				ptr = ret_data;
				size = 4;
				break;
			}
			case 0x0E: {
				pio_toggle_pin(udd_g_ctrlreq.req.wValue&0xFF);
				break;
			}
			case 0xBB: {
				flash_clear_gpnvm(1);
				reset = true;
				break;
			}
			case 0x5C: {
				int32_t x = twi_probe(TWI0, udd_g_ctrlreq.req.wIndex&0xFF) == TWI_SUCCESS;
				ret_data[0] = x;
				ptr = ret_data;
				size = 4;
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
