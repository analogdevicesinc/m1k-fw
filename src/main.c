#include <asf.h>
#include "conf_usb.h"

static bool main_b_vendor_enable = false;
#define stringify(x)            #x
#define xstringify(s) stringify(s)
bool reset = false;
uint8_t serial_number[USB_DEVICE_GET_SERIAL_NAME_LENGTH];
const char hwversion[] = xstringify(HW_VERSION);
const char fwversion[] = xstringify(FW_VERSION);
volatile uint32_t packet_offset = 0;

static  usart_spi_opt_t USART_SPI_ADC =
{
	.baudrate     = 24000000,
	.char_length   = US_MR_CHRL_8_BIT,
	.spi_mode      = SPI_MODE_0,
	.channel_mode  = US_MR_CHMODE_NORMAL
};

static  usart_spi_opt_t USART_SPI_DAC =
{
	.baudrate     = 24000000,
	.char_length   = US_MR_CHRL_8_BIT,
	.spi_mode      = SPI_MODE_1,
	.channel_mode  = US_MR_CHMODE_NORMAL
};

static twi_options_t TWIM_CONFIG =
{
	.master_clk = F_CPU,
	.speed = 100000,
	.chip = 0,
	.smbus = 0,
};

static uint32_t main_buf[1024];
uint16_t* sample_view = (uint16_t*)main_buf;
uint8_t* main_buf_loopback = (uint8_t*)main_buf;
uint8_t* ret_data = (uint8_t*)main_buf;

void init_build_usb_serial_number(void) {
	uint32_t uid[4];
	flash_read_unique_id(uid, 4);
	for (uint8_t i = 0; i < 16; i++) {
		serial_number[i*2+1] = "0123456789ABCDEF"[((uint8_t *)uid)[i]&0x0F];
		serial_number[i*2] = "0123456789ABCDEF"[(((uint8_t *)uid)[i]&0xF0) >> 4];
	}
	serial_number[32] = 0;
}

void TC1_Handler(void) {
	uint32_t active_conditions = TC0->TC_CHANNEL[1].TC_SR & 0x001C;
	active_conditions = active_conditions >> 2;
	switch (active_conditions) {
		// RA match, CNV
		case 1: {
			// CNV H->L
			pio_toggle_pin(26);
			// LDAC
			pio_toggle_pin(14);
			cpu_delay_us(1, F_CPU);
			pio_toggle_pin(14);
			break;
		}
		// RB match, move data
		case 2: {
			// SYNC H->L
			pio_toggle_pin(16);
			cpu_delay_us(4, F_CPU);
			// setup DAC out
			// USART0->US_TPR = &packets_out[packet_index].data[packet_offset*2];
			// USART0->US_TCR = 3;
			// setup ADC out
			// USART1->US_TPR = &packets_in[packet_index].ADC_conf;
			// USART1->US_TCR = 2;
			// USART1->US_RPR = &packets_in[packet_index].data[packet_offset*2];
			// USART1->US_RCR = 2;
			// USART2->US_TPR = &packets_in[packet_index].ADC_conf;
			// USART2->US_TCR = 2;
			// USART2->US_RPR = &packets_in[packet_index].data[packet_offset*2+1];
			// USART2->US_RCR = 2;
			// enable all the transactions
			// wait until DAC transaction complete
			// SYNC L->H
			pio_toggle_pin(16);
			cpu_delay_us(1, F_CPU);
			// SYNC H->L
			pio_toggle_pin(16);
			cpu_delay_us(4, F_CPU);
			// USART0->US_TPR = &packets_out[packet_index].data[packet_offset*2+1];
			// USART0->US_TCR = 3;
			// wait until ADC transfers complete
			// wait until DAC transfer completes
			// SYNC L->H
			pio_toggle_pin(16);
			// CNV L->H (after all transfers complete)
			pio_toggle_pin(26);
			packet_offset += 1;
			break;
		}
		// RC match, housekeeping and USB as needed
		case 4: {
			if (packet_offset > 511) {
				// send_packet(packets_out[packet_index]);
				// get_packet(packets_in[packet_index]);
				// packet_index = packet_index^1
				packet_offset = 0;
			}
			/* if (packet_offset == 1)
				packets_in[packet_index].ADC_conf = 0;
			*/
			break;
		}
	}
	
}

void hardware_init(void) {
// enable peripheral clock access
	pmc_enable_periph_clk(ID_PIOA);
	pmc_enable_periph_clk(ID_PIOB);
	pmc_enable_periph_clk(ID_TWI0);
	pmc_enable_periph_clk(ID_USART0);
	pmc_enable_periph_clk(ID_USART1);
	pmc_enable_periph_clk(ID_USART2);
	pmc_enable_periph_clk(ID_TC1);

// PWR
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB17, PIO_DEFAULT);

// SDA
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA9A_TWD0, PIO_DEFAULT);
// SCL
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA10A_TWCK0, PIO_DEFAULT);

// LDAC_N
	pio_configure(PIOA, PIO_OUTPUT_1, PIO_PA14, PIO_DEFAULT);
// CLR_N
	pio_configure(PIOA, PIO_OUTPUT_1, PIO_PA15, PIO_DEFAULT);
// SYNC_N
	pio_configure(PIOA, PIO_OUTPUT_1, PIO_PA16, PIO_DEFAULT);
// DAC_CLK
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA17A_SCK0, PIO_DEFAULT);
// DAC_MOSI
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA18A_TXD0, PIO_DEFAULT);

// CHA_ADC_MISO
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA21A_RXD1, PIO_DEFAULT);
// CHA_ADC_MOSI
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA20A_TXD1, PIO_DEFAULT);
// CHA_ADC_CLK
	pio_configure(PIOA, PIO_PERIPH_B, PIO_PA24B_SCK1, PIO_DEFAULT);

// ADC_CNV
	pio_configure(PIOA, PIO_OUTPUT_1, PIO_PA26, PIO_DEFAULT);

// CHB_ADC_MISO
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA22A_TXD2, PIO_DEFAULT);
// CHB_ADC_MOSI
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA23A_RXD2, PIO_DEFAULT);
// CHB_ADC_CLK
	pio_configure(PIOA, PIO_PERIPH_B, PIO_PA25B_SCK2, PIO_DEFAULT);

// CHA_SWMODE
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB19, PIO_DEFAULT);
// CHB_SWMODE
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB20, PIO_DEFAULT);

// CHA_SAFE_SWITCH
	pio_configure(PIOB, PIO_INPUT, PIO_PB4, PIO_DEFAULT);
// CHB_SAFE_SWITCH
	pio_configure(PIOB, PIO_INPUT, PIO_PB18, PIO_DEFAULT);

// CHA_OUT_CONNECT
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB0, PIO_DEFAULT);
// CHA_OUT_50OPAR
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB1, PIO_DEFAULT);
// CHA_OUT_10KSER
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB2, PIO_DEFAULT);
// CHA_OUT_1MOPAR
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB3, PIO_DEFAULT);

// CHB_OUT_CONNECT
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB5, PIO_DEFAULT);
// CHB_OUT_50OPAR
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB6, PIO_DEFAULT);
// CHB_OUT_10KSER
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB7, PIO_DEFAULT);
// CHB_OUT_1MOPAR
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB8, PIO_DEFAULT);

	usart_init_spi_master(USART0, &USART_SPI_DAC, F_CPU);
	usart_enable_tx(USART0);
	usart_init_spi_master(USART1, &USART_SPI_ADC, F_CPU);
	usart_enable_tx(USART1);
	usart_enable_rx(USART1);
	usart_init_spi_master(USART2, &USART_SPI_ADC, F_CPU);
	usart_enable_tx(USART2);
	usart_enable_rx(USART2);

// 100khz I2C
	twi_reset(TWI0);
	twi_enable_master_mode(TWI0);
	twi_master_init(TWI0, &TWIM_CONFIG);

	// TC1, channel 0 used for sampling
	// disable clock
	TC0->TC_CHANNEL[1].TC_CCR = TC_CCR_CLKDIS;
	// disable all interrupts
	TC0->TC_CHANNEL[1].TC_IDR = ~0;
	// clear status reg
	TC0->TC_CHANNEL[1].TC_SR;
	// config
	TC0->TC_CHANNEL[1].TC_CMR = TC_CMR_TCCLKS_TIMER_CLOCK4 | TC_CMR_WAVSEL_UP_RC | TC_CMR_WAVE;// 96MHz/128 -> 750kcps
	TC0->TC_CHANNEL[1].TC_RC = 0xffff; // 11Hz
	TC0->TC_CHANNEL[1].TC_RB = 0x7fff; // 20uS
	TC0->TC_CHANNEL[1].TC_RA = 0x000f; // 20uS
	TC0->TC_CHANNEL[1].TC_IER = TC_IER_CPAS | TC_IER_CPBS | TC_IER_CPCS;
	TC0->TC_CHANNEL[1].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG;
}

void write_dac(uint8_t cmd, uint8_t addr, uint16_t val) {
	uint32_t b[3];
	pio_toggle_pin(16);
	b[0] = cmd << 3 | addr;
	b[1] = val >> 8;
	b[2] = val >> 8;
	usart_putchar(USART0, b[0]);
	usart_putchar(USART0, b[1]);
	usart_putchar(USART0, b[2]);
	while(!((USART0->US_CSR & US_CSR_TXEMPTY) > 0));
	pio_toggle_pin(16);
	pio_toggle_pin(14);
	cpu_delay_us(1, F_CPU);
	pio_toggle_pin(14);
}

uint32_t read_adc(uint16_t cmd) {
	// RAC
	uint32_t hb, lb;
	pio_toggle_pin(26);
	usart_putchar(USART1, cmd >> 8);
	while(!((USART1->US_CSR & US_CSR_TXEMPTY) > 0));
	usart_read(USART1, &hb);
	usart_putchar(USART1, cmd & 0xff);
	while(!((USART1->US_CSR & US_CSR_TXEMPTY) > 0));
	usart_read(USART1, &lb);
	uint32_t x = (hb << 8 | lb);
	pio_toggle_pin(26);
	return x;
}

void read_adcs(uint32_t count, uint16_t* out) {
	for (uint32_t i = 0; i < count; i++) {
		pio_toggle_pin(26);
		for (uint32_t j = 0; j < 2; j++) {
			while(!((USART1->US_CSR & US_CSR_TXRDY) > 0));
			USART1->US_THR = US_THR_TXCHR(0);
			while(!((USART1->US_CSR & US_CSR_RXRDY) > 0));
			main_buf_loopback[i*2+j] = USART1->US_RHR & US_RHR_RXCHR_Msk;
		}
		while(!((USART1->US_CSR & US_CSR_TXEMPTY) > 0));
		pio_toggle_pin(26);
		cpu_delay_us(2, F_CPU);
	}
}

void write_pots(uint8_t ch, uint8_t r1, uint8_t r2) {
	twi_packet_t p;
	uint8_t v;
	if (ch == 'a') {
		p.chip = 0x2f;
	}
	if (ch == 'b') {
		p.chip = 0x23;
	}
	p.length = 1;
	p.addr_length = 1;
	p.buffer = &v;
	p.addr[0] = 0x10;
	v = r1&0x7f;
	twi_master_write(TWI0, &p);
	p.addr[0] = 0x11;
	v = r2&0x7f;
	twi_master_write(TWI0, &p);
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

	udc_detach();
	udc_stop();
	udc_start();
	cpu_delay_us(10, F_CPU);
	udc_attach();
	while (true) {
		cpu_delay_us(100, F_CPU);
		if (!reset)
			wdt_restart(WDT);
		if (reset)
			udc_detach();
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
				}
				break;
			}
			case 0xEE: {
				Pio *p_pio = (Pio *)((uint32_t)PIOA + (PIO_DELTA * ((udd_g_ctrlreq.req.wValue&0xFF) >> 5)));
				ret_data[0] = (p_pio->PIO_ODSR & (1 << (udd_g_ctrlreq.req.wValue& 0x1F))) != 0;
				ptr = ret_data;
				size = 1;
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
			case 0x50: {
				uint8_t c = udd_g_ctrlreq.req.wValue&0xFF;
				usart_putchar(USART0, c);
				break;
			}
			case 0x3D: {
				uint8_t cmd = udd_g_ctrlreq.req.wIndex&0xFF;
				uint8_t addr = (udd_g_ctrlreq.req.wIndex>>8)&0xFF;
				write_dac(cmd, addr, udd_g_ctrlreq.req.wValue);
				break;
			}
			case 0x1B: {
				uint8_t r1 = udd_g_ctrlreq.req.wValue&0xFF;
				uint8_t r2 = (udd_g_ctrlreq.req.wValue>>8)&0xFF;
				uint8_t ch = udd_g_ctrlreq.req.wIndex&0xFF;
				write_pots(ch, r1, r2);
				break;
			}
			case 0xAD: {
				uint16_t cmd = udd_g_ctrlreq.req.wIndex;
				uint16_t val = read_adc(cmd);
				ret_data[0] = val&0xff;
				ret_data[1] = val >> 8;
				ptr = ret_data;
				size = 2;
				break;
			}
			case 0xA4: {
				uint32_t ct = udd_g_ctrlreq.req.wIndex;
				read_adcs(ct, sample_view);
				size = ct*2;
				ptr = ret_data;
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
