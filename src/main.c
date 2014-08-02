#include <asf.h>
#include "conf_usb.h"
#include "conf_board.h"

static bool main_b_vendor_enable = false;
#define stringify(x)            #x
#define xstringify(s) stringify(s)
bool reset = false;
uint8_t serial_number[USB_DEVICE_GET_SERIAL_NAME_LENGTH];
const char hwversion[] = xstringify(HW_VERSION);
const char fwversion[] = xstringify(FW_VERSION);
volatile uint32_t slot_offset = 0;
uint8_t* ret_data[16];
uint16_t va = 0;
uint16_t vb = 0;
uint16_t ia = 0;
uint16_t ib = 0;


#define num_packets 2

volatile bool sampling = 0;

/// Packet index used for sampling
volatile unsigned out_sample_packet = 0;
/// Next packet index to be received
volatile unsigned out_transfer_packet = 0;
/// Number of packets available to read
volatile unsigned out_packets_available = 0;


/// Packet index used for sampling
volatile unsigned in_sample_packet = 0;
/// Next packet index to be sent
volatile unsigned in_transfer_packet = 0;
/// Number of packets available to write
volatile unsigned in_packets_available = 0;

/// Transfer currently active
volatile bool out_active = false;
volatile bool in_active = false;

IN_packet packets_in[num_packets];
OUT_packet packets_out[num_packets];

/// 
volatile bool in_sending = 0;
volatile bool out_sending = 0;

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

void init_build_usb_serial_number(void) {
	uint32_t uid[4];
	flash_read_unique_id(uid, 4);
	for (uint8_t i = 0; i < 16; i++) {
		serial_number[i*2+1] = "0123456789ABCDEF"[((uint8_t *)uid)[i]&0x0F];
		serial_number[i*2] = "0123456789ABCDEF"[(((uint8_t *)uid)[i]&0xF0) >> 4];
	}
}

void TC1_Handler(void) {
	uint32_t stat = tc_get_status(TC0, 1);
	if ((stat & TC_SR_CPCS) > 0) {
		pio_set(PIOA, IO0);
		
		unsigned out_index = out_sample_packet;
		unsigned in_index = in_sample_packet;

		if (out_packets_available == 0 || in_packets_available == num_packets) {
			return;
		}
		
		// SYNC & CNV H->L
		pio_clear(PIOA, CNV|N_SYNC);
		USART0->US_TPR = &packets_out[out_index].data_a[slot_offset];
		USART1->US_TPR = &va;
		USART1->US_RPR = &packets_in[in_index].data_a_v[slot_offset];
		USART2->US_TPR = &vb;
		USART2->US_RPR = &packets_in[in_index].data_b_v[slot_offset];
		USART0->US_TCR = 3;
		USART1->US_TCR = 2;
		USART1->US_RCR = 2;
		USART2->US_TCR = 2;
		USART2->US_RCR = 2;
		// wait until transactions complete
		while(!((USART2->US_CSR&US_CSR_TXEMPTY) > 0));
		while(!((USART0->US_CSR&US_CSR_TXEMPTY) > 0));
		// strobe SYNC, CNV out of phase for next words
		// both need to be toggled between channel interactions
		// cnv should not be \pm 20ns of a dio change
		pio_set(PIOA, CNV);
		cpu_delay_us(3, F_CPU);
		pio_clear(PIOA, CNV);
		USART1->US_TPR = &ia;
		USART1->US_RPR = &packets_in[in_index].data_a_i[slot_offset];
		USART2->US_TPR = &ib;
		USART2->US_RPR = &packets_in[in_index].data_b_i[slot_offset];
		USART1->US_TCR = 2;
		USART1->US_RCR = 2;
		USART2->US_TCR = 2;
		USART2->US_RCR = 2;
		// wait until transfers completes
		while(!((USART2->US_CSR&US_CSR_TXEMPTY) > 0));
		cpu_delay_us(1, F_CPU);
		// SYNC & CNV L->H (after all transfers complete)
		pio_set(PIOA, CNV|N_SYNC);
		pio_clear(PIOA, N_LDAC);
		pio_set(PIOA, N_LDAC);
		slot_offset += 1;
		pio_clear(PIOA, IO3);
		if (slot_offset > 255) {
			out_packets_available--;
			in_packets_available++;
			
			out_sample_packet = (out_sample_packet + 1) % num_packets;
			in_sample_packet  = (in_sample_packet + 1) % num_packets;
			
			slot_offset = 0;			
		}
		pio_clear(PIOA, IO0);
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


// GPIO
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA0, PIO_DEFAULT);
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA1, PIO_DEFAULT);
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA2, PIO_DEFAULT);
	pio_configure(PIOA, PIO_OUTPUT_0, PIO_PA3, PIO_DEFAULT);


// LED
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB10, PIO_DEFAULT);
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB9, PIO_DEFAULT);

// PWR
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB17, PIO_DEFAULT);

// SDA
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA9A_TWD0, PIO_DEFAULT);
// SCL
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA10A_TWCK0, PIO_DEFAULT);

// DAC
// LDAC_N
	pio_configure(PIOA, PIO_OUTPUT_1, PIO_PA14, PIO_DEFAULT);
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
	pio_configure(PIOA, PIO_OUTPUT_1, PIO_PA26, PIO_DEFAULT);

// CHB_ADC
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA22A_TXD2, PIO_DEFAULT);
	pio_configure(PIOA, PIO_PERIPH_A, PIO_PA23A_RXD2, PIO_DEFAULT);
	pio_configure(PIOA, PIO_PERIPH_B, PIO_PA25B_SCK2, PIO_DEFAULT);

// CHA_SWMODE
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB19, PIO_DEFAULT);
// CHB_SWMODE
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB20, PIO_DEFAULT);

// CHA_OUT_1KOPAR
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB0, PIO_DEFAULT);
// CHA_OUT_50OPAR
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB1, PIO_DEFAULT);
// CHA_OUT_10KSER
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB2, PIO_DEFAULT);
// CHA_OUT_1MOPAR
	pio_configure(PIOB, PIO_OUTPUT_0, PIO_PB3, PIO_DEFAULT);

// CHA_OUT_1KOPAR
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB5, PIO_DEFAULT);
// CHB_OUT_50OPAR
	pio_configure(PIOB, PIO_OUTPUT_1, PIO_PB6, PIO_DEFAULT);
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
// enable pdc for USARTs
	USART0->US_PTCR = US_PTCR_TXTEN;
	USART1->US_PTCR = US_PTCR_TXTEN | US_PTCR_RXTEN;
	USART2->US_PTCR = US_PTCR_TXTEN | US_PTCR_RXTEN;

// 100khz I2C
	twi_reset(TWI0);
	twi_enable_master_mode(TWI0);
	twi_master_init(TWI0, &TWIM_CONFIG);

// CLOCK3 = MCLK/32
// counts to RC
	tc_init(TC0, 1, TC_CMR_TCCLKS_TIMER_CLOCK3 | TC_CMR_WAVSEL_UP_RC | TC_CMR_WAVE);
	tc_enable_interrupt(TC0, 1, TC_IER_CPAS | TC_IER_CPCS);
	NVIC_EnableIRQ(TC1_IRQn);
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
	// enable WDT for 0.5s
	wdt_init(WDT, WDT_MR_WDRSTEN, 500000, 500000);
	// setup peripherals
	hardware_init();
	// start USB

	udc_detach();
	udc_stop();
	udc_start();
	cpu_delay_us(10, F_CPU);
	udc_attach();
	while (true) {
		if (sampling) {
			if (!in_active && in_packets_available != 0) {
				in_active = true;
				udi_vendor_bulk_in_run((uint8_t *)&(packets_in[in_transfer_packet]), sizeof(IN_packet), main_vendor_bulk_in_received);
			}
			
			if (!out_active && out_packets_available != num_packets) {
				out_active = true;
				udi_vendor_bulk_out_run((uint8_t *)&(packets_out[out_transfer_packet]), sizeof(OUT_packet), main_vendor_bulk_out_received);
			}
		} else {
			if (in_active) {
				udd_ep_abort(UDI_VENDOR_EP_BULK_IN);
			}
			if (out_active) {
				udd_ep_abort(UDI_VENDOR_EP_BULK_OUT);
			}
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
			case 0x1B: {
				uint8_t r1 = udd_g_ctrlreq.req.wValue&0xFF;
				uint8_t r2 = (udd_g_ctrlreq.req.wValue>>8)&0xFF;
				uint8_t ch = udd_g_ctrlreq.req.wIndex&0xFF;
				write_pots(ch, r1, r2);
				break;
			}
			case 0xCA: { // config A
				va = Swap16(udd_g_ctrlreq.req.wValue);
				ia = Swap16(udd_g_ctrlreq.req.wIndex);
				break;
			}
			case 0xCB: {
				vb = Swap16(udd_g_ctrlreq.req.wValue);
				ib = Swap16(udd_g_ctrlreq.req.wIndex);
				break;
			}
			case 0xC5: {
				if (udd_g_ctrlreq.req.wValue < 1) {
					tc_stop(TC0, 1);
					sampling = false;
				} else {
					sampling = 1;
					out_sample_packet = 0;
					out_transfer_packet = 0;
					out_packets_available = 0;

					in_sample_packet = 0;
					in_transfer_packet = 0;
					in_packets_available = 0;
					
					slot_offset = 0;
					tc_write_ra(TC0, 1, udd_g_ctrlreq.req.wValue);
					tc_write_rc(TC0, 1, udd_g_ctrlreq.req.wIndex);
					tc_start(TC0, 1);
				}
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
	in_active = false;
	if (UDD_EP_TRANSFER_OK != status) {
		return;
	} else {
		in_transfer_packet = (in_transfer_packet + 1) % num_packets;
		in_packets_available -= 1;
	}
}

void main_vendor_bulk_out_received(udd_ep_status_t status,
		iram_size_t nb_transfered, udd_ep_id_t ep)
{
	UNUSED(ep);
	out_active = false;
	if (UDD_EP_TRANSFER_OK != status) {
		return;
	} else {
		out_transfer_packet = (out_transfer_packet + 1) % num_packets;
		out_packets_available += 1;
	}
}
