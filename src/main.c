#include <asf.h>
#include "conf_usb.h"
#include "conf_board.h"

#define stringify(x)			#x
#define xstringify(s) stringify(s)
static bool main_b_vendor_enable;
uint8_t serial_number[USB_DEVICE_GET_SERIAL_NAME_LENGTH];
const char hwversion[] = xstringify(HW_VERSION);
const char fwversion[] = xstringify(FW_VERSION);
volatile uint32_t slot_offset;
volatile uint32_t packet_index_in;
volatile uint32_t packet_index_out;
volatile uint32_t packet_index_send_in;
volatile uint32_t packet_index_send_out;
volatile bool send_in;
volatile bool send_out;
volatile bool sending_in;
volatile bool sending_out;
volatile bool sent_in;
volatile bool sent_out;
volatile bool channel_a;
volatile bool reset;

chan_mode ma = DISABLED;
chan_mode mb = DISABLED;
uint8_t ret_data[16];

uint16_t i0_dacA = 26100;
uint16_t i0_dacB = 26100;
uint16_t va = 0x20F1;
uint16_t ia = 0x20F5;
uint16_t vb = 0x20F1;
uint16_t ib = 0x20F5;
uint8_t da = 0;
uint8_t db = 1;

pwm_channel_t PWM_CH;

static pwm_clock_t PWM_SETTINGS = {
	.ul_clka = 1e6,
	.ul_clkb = 0,
	.ul_mck = F_CPU
};

static  usart_spi_opt_t USART_SPI_ADC =
{
	.baudrate	 = 24000000,
	.char_length   = US_MR_CHRL_8_BIT,
	.spi_mode	  = SPI_MODE_0,
	.channel_mode  = US_MR_CHMODE_NORMAL | US_MR_INACK
};

static  usart_spi_opt_t USART_SPI_DAC =
{
	.baudrate	 = 24000000,
	.char_length   = US_MR_CHRL_8_BIT,
	.spi_mode	  = SPI_MODE_1,
	.channel_mode  = US_MR_CHMODE_NORMAL
};

static twi_options_t TWIM_CONFIG =
{
	.master_clk = F_CPU,
	.speed = 100000,
	.chip = 0,
	.smbus = 0,
};

/* Credit to Tod E. Kurt, ThingM, tod@todbot.com
 * Given a variable hue 'h', that ranges from 0-252,
 * set RGB color value appropriately.
 * Assumes maximum Saturation & maximum Value (brightness)
 * Performs purely integer math, no floating point.
 */
void h_to_rgb(uint8_t h, rgb* c) 
{
	uint8_t hd = h / 42;   // 42 == 252/6,  252 == H_MAX
	uint8_t hi = hd % 6;   // gives 0-5
	uint8_t f = h % 42; 
	uint8_t fs = f * 6;
	switch( hi ) {
		case 0:
			c->r = 252;	 c->g = fs;	  c->b = 0;
		   break;
		case 1:
			c->r = 252-fs;  c->g = 252;	 c->b = 0;
			break;
		case 2:
			c->r = 0;	   c->g = 252;	 c->b = fs;
			break;
		case 3:
			c->r = 0;	   c->g = 252-fs;  c->b = 252;
			break;
		case 4:
			c->r = fs;	  c->g = 0;	   c->b = 252;
			break;
		case 5:
			c->r = 252;	 c->g = 0;	   c->b = 252-fs;
			break;
	}
}

void init_build_usb_serial_number(void) {
	uint32_t uid[4];
	flash_read_unique_id(uid, 4);
	for (uint8_t i = 0; i < 16; i++) {
		serial_number[i*2+1] = "0123456789ABCDEF"[((uint8_t *)uid)[i]&0x0F];
		serial_number[i*2] = "0123456789ABCDEF"[(((uint8_t *)uid)[i]&0xF0) >> 4];
	}
}

void TC2_Handler(void) {
	uint32_t stat = tc_get_status(TC0, 2);
	if (!sent_out)
		return;
	if ((stat & TC_SR_CPCS) > 0) {
		if (channel_a) {
			USART0->US_TPR = (uint32_t)(&da);
			USART0->US_TNPR = (uint32_t)(&packets_out[packet_index_out].data_a[slot_offset]);
			USART1->US_TPR = (uint32_t)(&va);
			USART1->US_RPR = (uint32_t)(&packets_in[packet_index_in].data_a_v[slot_offset]);
			USART2->US_TPR = (uint32_t)(&vb);
			USART2->US_RPR = (uint32_t)(&packets_in[packet_index_in].data_b_v[slot_offset]);
			pio_clear(PIOA, N_SYNC);
			USART0->US_TCR = 1;
			USART0->US_TNCR = 2;
			USART1->US_RCR = 2;
			USART1->US_TCR = 2;
			USART2->US_RCR = 2;
			USART2->US_TCR = 2;
			while(!((USART2->US_CSR&US_CSR_ENDRX) > 0));
			while(!((USART0->US_CSR&US_CSR_TXEMPTY) > 0));
			pio_set(PIOA, N_SYNC);
			channel_a ^= true;
			return;
		}
		if (!channel_a) {
			USART0->US_TPR = (uint32_t)(&db);
			USART0->US_TNPR = (uint32_t)(&packets_out[packet_index_out].data_b[slot_offset]);
			USART1->US_TPR = (uint32_t)(&ia);
			USART1->US_RPR = (uint32_t)(&packets_in[packet_index_in].data_a_i[slot_offset]);
			USART2->US_TPR = (uint32_t)(&ib);
			USART2->US_RPR = (uint32_t)(&packets_in[packet_index_in].data_b_i[slot_offset]);
			pio_clear(PIOA, N_SYNC);
			USART0->US_TCR = 1;
			USART0->US_TNCR = 2;
			USART1->US_TCR = 2;
			USART1->US_RCR = 2;
			USART2->US_TCR = 2;
			USART2->US_RCR = 2;
			while(!((USART2->US_CSR&US_CSR_ENDRX) > 0));
			while(!((USART0->US_CSR&US_CSR_TXEMPTY) > 0));
			slot_offset += 1;
			channel_a ^= true;
			pio_set(PIOA, N_SYNC);
		}
		if (slot_offset == 127) {
			packet_index_send_out = packet_index_out^1;
			send_out = true;
		}
		if (slot_offset > 255) {
			slot_offset = 0;
			packet_index_send_in = packet_index_in;
			packet_index_in ^= 1;
			packet_index_out ^= 1;
			send_in = true;
		}
	}
}

void init_hardware(void) {
// enable peripheral clock access
	pmc_enable_periph_clk(ID_PIOA);
	pmc_enable_periph_clk(ID_PIOB);
	pmc_enable_periph_clk(ID_TWI0);
	pmc_enable_periph_clk(ID_USART0);
	pmc_enable_periph_clk(ID_USART1);
	pmc_enable_periph_clk(ID_USART2);
	pmc_enable_periph_clk(ID_TC0);
	pmc_enable_periph_clk(ID_TC1);
	pmc_enable_periph_clk(ID_PWM);
	pmc_enable_periph_clk(ID_TC2);


// GPIO
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA0, PIO_DEFAULT);
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA1, PIO_DEFAULT);
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA2, PIO_DEFAULT);
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA3, PIO_DEFAULT);


// LED
	pio_configure(PIOA, PIO_PERIPH_B, PIO_PA28, PIO_DEFAULT);
	pio_configure(PIOA, PIO_PERIPH_B, PIO_PA29, PIO_DEFAULT);
	pio_configure(PIOB, PIO_PERIPH_B, PIO_PB15, PIO_DEFAULT);

// PWR
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB17, PIO_DEFAULT);

// SDA
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA9A_TWD0, PIO_DEFAULT);
// SCL
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA10A_TWCK0, PIO_DEFAULT);

// DAC
// LDAC_N
	pio_configure(PIOA, PIO_PERIPH_B, PIO_PA30, PIO_DEFAULT);
// CLR_N
	pio_configure(PIOA, PIO_OUTPUT_1, PIO_PA15, PIO_DEFAULT);
// SYNC_N
	pio_configure(PIOA, PIO_OUTPUT_1, PIO_PA16, PIO_DEFAULT);
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA17A_SCK0, PIO_DEFAULT);
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA18A_TXD0, PIO_DEFAULT);

// CHA_ADC
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA21A_RXD1, PIO_DEFAULT);
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA20A_TXD1, PIO_DEFAULT);
	pio_configure(PIOA, PIO_PERIPH_B, PIO_PA24B_SCK1, PIO_DEFAULT);

// ADC_CNV
	pio_configure(PIOA, PIO_PERIPH_B, PIO_PA31, PIO_DEFAULT);

// CHB_ADC
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA22A_TXD2, PIO_DEFAULT);
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA23A_RXD2, PIO_DEFAULT);
	pio_configure(PIOA, PIO_PERIPH_B, PIO_PA25B_SCK2, PIO_DEFAULT);

// CHA_SWMODE
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB19, PIO_DEFAULT);
// CHB_SWMODE
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB20, PIO_DEFAULT);

// 50o to 2v5
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB0, PIO_DEFAULT);
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB5, PIO_DEFAULT);
// 50o to gnd
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB1, PIO_DEFAULT);
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB6, PIO_DEFAULT);
// feedback / sense
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB2, PIO_DEFAULT);
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB7, PIO_DEFAULT);
// output
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB3, PIO_DEFAULT);
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB8, PIO_DEFAULT);


	usart_init_spi_master(USART0, &USART_SPI_DAC, F_CPU);
	usart_enable_tx(USART0);
	usart_init_spi_master(USART1, &USART_SPI_ADC, F_CPU);
	usart_enable_tx(USART1);
	usart_enable_rx(USART1);
	usart_init_spi_master(USART2, &USART_SPI_ADC, F_CPU);
	usart_enable_tx(USART2);
	usart_enable_rx(USART2);
// enable pdc for USARTs
	USART0->US_PTCR = US_PTCR_TXTEN;
	USART1->US_PTCR = US_PTCR_TXTEN | US_PTCR_RXTEN;
	USART2->US_PTCR = US_PTCR_TXTEN | US_PTCR_RXTEN;


// 100khz I2C
	twi_reset(TWI0);
	twi_enable_master_mode(TWI0);
	twi_master_init(TWI0, &TWIM_CONFIG);


// CLOCK3 = MCLK/32
// RA takes LDAC H->L
// RB takes CNV L->H
// RC takes CNV H->L, LDAC L->H
	tc_init(TC0, 2, TC_CMR_TCCLKS_TIMER_CLOCK3 | TC_CMR_WAVSEL_UP_RC | TC_CMR_WAVE | TC_CMR_ACPA_SET | TC_CMR_ACPC_CLEAR | TC_CMR_BCPB_SET | TC_CMR_BCPC_CLEAR | TC_CMR_EEVT_XC0 );
	tc_enable_interrupt(TC0, 2, TC_IER_CPAS | TC_IER_CPCS);
	NVIC_EnableIRQ(TC2_IRQn);


// set RGB LED to hue generated from UID
	uint32_t uid[4];
	rgb c;
	flash_read_unique_id(uid, 4);
	uint8_t h = uid[3] % 252;
	h_to_rgb(h, &c);


	pwm_channel_disable(PWM, PWM_CHANNEL_0); // PA28 - blue - PWMH0
	pwm_channel_disable(PWM, PWM_CHANNEL_1); // PA29 - green - PWMH1
	pwm_channel_disable(PWM, PWM_CHANNEL_2); // PB15 - red - PWMH2
	pwm_init(PWM, &PWM_SETTINGS);
	PWM_CH.ul_prescaler = PWM_CMR_CPRE_CLKA;
	PWM_CH.ul_period = 256;
	PWM_CH.ul_duty = c.b<<3;
	PWM_CH.channel = PWM_CHANNEL_0;
	pwm_channel_init(PWM, &PWM_CH);
	PWM_CH.ul_duty = c.g<<3;
	PWM_CH.channel = PWM_CHANNEL_1;
	pwm_channel_init(PWM, &PWM_CH);
	PWM_CH.ul_duty = c.r<<3;
	PWM_CH.channel = PWM_CHANNEL_2;
	pwm_channel_init(PWM, &PWM_CH);
	pwm_channel_enable(PWM, PWM_CHANNEL_0);
	pwm_channel_enable(PWM, PWM_CHANNEL_1);
	pwm_channel_enable(PWM, PWM_CHANNEL_2);

}

void config_hardware() {
	// continuous V&I conversion
	write_adm1177(0b00010101);
	cpu_delay_us(100, F_CPU);
	// sane simv
	write_ad5122(0, 0x30, 0x40);
	cpu_delay_us(100, F_CPU);
	write_ad5122(1, 0x30, 0x40);
	cpu_delay_us(100, F_CPU);
	// DAC internal reference
	write_ad5663(0xFF, 0xFFFF);
}

void write_ad5122(uint32_t ch, uint8_t r1, uint8_t r2) {
	twi_packet_t p;
	
	uint8_t v;
	if (ch == A) {
		p.chip = 0x2f;
	}
	if (ch == B) {
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

void write_adm1177(uint8_t v) {
	twi_packet_t p;
	p.chip = 0x58; // 7b addr of '1177 w/ addr p grounded
	p.addr_length = 1;
	p.addr[0] = v;
	p.length = 0;
	twi_master_write(TWI0, &p);

}

void read_adm1177(uint8_t* b, uint8_t ct) {
	twi_packet_t p;
	p.chip = 0x58;
	p.length = ct;
	p.buffer = b;
	p.addr_length = 0;
	twi_master_read(TWI0, &p);
}

void write_ad5663(uint8_t conf, uint16_t data) {
	USART0->US_TPR = (uint32_t)(&conf);
	USART0->US_TNPR = (uint32_t)(&data);
	pio_clear(PIOA, N_SYNC);
	cpu_delay_us(10, F_CPU);
	USART0->US_TCR = 1;
	USART0->US_TNCR = 2;
	while(!((USART0->US_CSR&US_CSR_TXEMPTY) > 0));
	cpu_delay_us(100, F_CPU);
	pio_set(PIOA, N_SYNC);
}

void set_mode(uint32_t chan, chan_mode m) {
	switch (chan) {
		case A: {
			switch (m) {
				case DISABLED: {
					ma = DISABLED;
					pio_set(PIOB, PIO_PB19); // simv
					pio_clear(PIOB, PIO_PB2);
					pio_set(PIOB, PIO_PB3);
					write_ad5663(0, i0_dacA);
					break;
					}
				case SVMI: {
					ma = SVMI;
					pio_clear(PIOB, PIO_PB19); // modeswitch = svmi
					pio_clear(PIOB, PIO_PB2);
					pio_clear(PIOB, PIO_PB3); // enable output
					break;
				}
				case SIMV: {
					ma = SIMV;
					pio_set(PIOB, PIO_PB19); // simv
					pio_clear(PIOB, PIO_PB2);
					pio_clear(PIOB, PIO_PB3); // enable output
					break;
				}
				default: {}
			}
			break;
		}
		case B: {
			switch (m) {
				case DISABLED: {
					mb = DISABLED;
					pio_set(PIOB, PIO_PB20); // simv
					pio_clear(PIOB, PIO_PB7);
					pio_set(PIOB, PIO_PB8); // disconnect output
					write_ad5663(1, i0_dacB);
					break;
					}
				case SVMI: {
					mb = SVMI;
					pio_clear(PIOB, PIO_PB20); // modeswitch = svmi
					pio_clear(PIOB, PIO_PB7);
					pio_clear(PIOB, PIO_PB8); // enable output
					break;
				}
				case SIMV: {
					mb = SIMV;
					pio_set(PIOB, PIO_PB20); // simv
					pio_clear(PIOB, PIO_PB7);
					pio_clear(PIOB, PIO_PB8); // enable output
					break;
				}
				default: {}
			}
			break;
		}
		default : {}
	}
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
	init_hardware();
	// start USB
	cpu_delay_us(100, F_CPU);

	udc_detach();
	udc_stop();
	udc_start();
	cpu_delay_us(10, F_CPU);
	udc_attach();
	write_ad5663(0, i0_dacA);
	write_ad5663(1, i0_dacB);
	while (true) {
		if ((!sending_in) & send_in) {
			send_in = false;
			sending_in = true;
			udi_vendor_bulk_in_run((uint8_t *)&(packets_in[packet_index_send_in]), sizeof(IN_packet), main_vendor_bulk_in_received);
		}
		if ((!sending_out) & send_out) {
			send_out = false;
			sending_out = true;
			udi_vendor_bulk_out_run((uint8_t *)&(packets_out[packet_index_send_out]), sizeof(OUT_packet), main_vendor_bulk_out_received);
		}
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
	uint16_t size = 0;
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
			case 0x17: {
				size = udd_g_ctrlreq.req.wIndex&0xFF;
				read_adm1177(&ret_data, size);
				ptr = (uint8_t*)&ret_data;
				break;
			}
			case 0x50: {
				pio_set_pin_low(udd_g_ctrlreq.req.wValue&0xFF);
				break;
			}
			case 0x51: {
				pio_set_pin_high(udd_g_ctrlreq.req.wValue&0xFF);
				break;
			}
			case 0x53: {
				set_mode(udd_g_ctrlreq.req.wValue&0xF, udd_g_ctrlreq.req.wIndex&0xF);
				break;
			}
			case 0xBB: {
				flash_clear_gpnvm(1);
				reset = true;
				break;
			}
			case 0xCC: {
				config_hardware();
				break;
			}
			case 0xC5: {
				if (udd_g_ctrlreq.req.wValue < 1) { 
					tc_stop(TC0, 2);
				}
				else {
					// how much state to reset?
					udd_ep_abort(UDI_VENDOR_EP_BULK_IN);
					udd_ep_abort(UDI_VENDOR_EP_BULK_OUT);

					channel_a = true;
					sent_out = false;
					sent_in = false;
					sending_in = false;
					sending_out = false;
					send_out = true;
					send_in = false;
					slot_offset = 0;
					packet_index_in = 0;
					packet_index_out = 0;
					packet_index_send_out = 0;
					packet_index_send_in = 0;
					// so much
					tc_write_ra(TC0, 2, udd_g_ctrlreq.req.wValue);
					tc_write_rb(TC0, 2, udd_g_ctrlreq.req.wIndex-udd_g_ctrlreq.req.wIndex/5);
					tc_write_rc(TC0, 2, udd_g_ctrlreq.req.wIndex);
				}
				break;
			}
		}
	}
	udd_g_ctrlreq.payload_size = size;
	if ( size == 0 ) {
		udd_g_ctrlreq.callback = 0;
		udd_g_ctrlreq.over_under_run = 0;
	}
	else {
		udd_g_ctrlreq.payload = ptr;
	}
	return true;
}

void main_vendor_bulk_in_received(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep)
{
	UNUSED(nb_transfered);
	UNUSED(ep);
	if (UDD_EP_TRANSFER_OK != status) {
		return;
	}
	else {
		sending_in = false;
	}
}

void main_vendor_bulk_out_received(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep)
{
	UNUSED(ep);
	if (UDD_EP_TRANSFER_OK != status) {
		return;
	}
	else {
		if (sent_out == false) {
			tc_start(TC0, 2);
		}
		sent_out = true;
		sending_out = false;
	}
}
