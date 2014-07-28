#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <libusb-1.0/libusb.h>
#include <math.h>
#include <endian.h>

const size_t chunk_size = 256;

extern "C" void LIBUSB_CALL in_completion(libusb_transfer *t);
extern "C" void LIBUSB_CALL out_completion(libusb_transfer *t);

struct Transfers {
	std::vector<libusb_transfer*> m_transfers;
	
	void alloc(unsigned count, libusb_device_handle* handle,
	           unsigned char endpoint, unsigned char type, size_t buf_size,
	           unsigned timeout, libusb_transfer_cb_fn callback, void* user_data) {
		m_transfers.resize(count, NULL);
		for (size_t i=0; i<count; i++) {
			auto t = m_transfers[i] = libusb_alloc_transfer(0);
			t->dev_handle = handle;
			t->flags = LIBUSB_TRANSFER_FREE_BUFFER;
			t->endpoint = endpoint;
			t->type = type;
			t->timeout = timeout;
			t->length = buf_size;
			t->callback = callback;
			t->user_data = user_data;
			t->buffer = (uint8_t*) malloc(buf_size);
		}
	}
	
	void clear() {
		for (auto i: m_transfers) {
			libusb_free_transfer(i);
		}
		m_transfers.clear();
	}
	
	size_t size() {
		return m_transfers.size();
	}
	
	~Transfers() {
		clear();
	}
	
	typedef std::vector<libusb_transfer*>::iterator iterator;
	typedef std::vector<libusb_transfer*>::const_iterator const_iterator;
	iterator begin() { return m_transfers.begin(); }
	const_iterator begin() const { return m_transfers.begin(); }
	iterator end() { return m_transfers.end(); }
	const_iterator end() const { return m_transfers.end(); }
};

struct HeliumDevice {
	HeliumDevice(libusb_device_handle* handle): m_usb(handle) {}
	~HeliumDevice() {
		libusb_close(m_usb);
	}
	
	void claim() {
		libusb_claim_interface(m_usb, 0);
		libusb_set_interface_alt_setting(m_usb, 0, 1);
	}
	
	void release() {
		libusb_release_interface(m_usb, 0);
	}
	
	void config_sync(uint64_t sample_rate, uint64_t sample_count) {
		m_sample_rate = sample_rate;
		m_sample_count = sample_count;
		m_in_transfers.alloc(6, m_usb, 0x81, LIBUSB_TRANSFER_TYPE_BULK, 1024, 1000, in_completion, this);
		m_out_transfers.alloc(6, m_usb, 0x02, LIBUSB_TRANSFER_TYPE_BULK, 2048, 1000, out_completion, this);
	}
	
	void start() {
		uint8_t buf[4];
		// set pots for sane simv
		libusb_control_transfer(m_usb, 0x40|0x80, 0x1B, 0x0707, 'a', buf, 4, 100);
		// set adcc for bipolar sequenced mode
		libusb_control_transfer(m_usb, 0x40|0x80, 0xAD, 0xF5CC, 0, buf, 1, 100);
		// set timer for 1us keepoff, 20us period
		libusb_control_transfer(m_usb, 0x40|0x80, 0xC5, 0x0004, 0x003E, buf, 1, 100);
		
		std::lock_guard<std::mutex> lock(m_state);
		m_requested_sampleno = m_in_sampleno = m_out_sampleno = 0;
		
		for (auto i: m_in_transfers) {
			if (!submit_in_transfer(i)) break;
		}
		
		for (auto i: m_out_transfers) {
			if (!submit_out_transfer(i)) break;
		}
	}
	void stop() {
		uint8_t buf[4];
		libusb_control_transfer(m_usb, 0x40|0x80, 0xC5, 0x0000, 0x0000, buf, 1, 100);
		libusb_control_transfer(m_usb, 0x40|0x80, 0x3D, 0x0000, 0x0000, buf, 1, 100);
	}
	
	bool submit_out_transfer(libusb_transfer* t) {
		if (m_sample_count == 0 || m_out_sampleno < m_sample_count + 512) { //TODO: firmware bug that we have to send an extra packet
			std::cerr << "submit_out_transfer " << m_out_sampleno << std::endl;
			auto buf = (uint32_t*) t->buffer;
			for (size_t i = 0; i < chunk_size; i++) {
				buf[i] = buf[i+chunk_size] = htobe16(m_src_buf[m_out_sampleno++]) << 8;
			}
			
			int r = libusb_submit_transfer(t);
			return true;
		}
		return false;
	}
	
	bool submit_in_transfer(libusb_transfer* t) {
		if (m_sample_count == 0 || m_requested_sampleno < m_sample_count) {
			std::cerr << "submit_in_transfer " << m_requested_sampleno << std::endl;
			int r = libusb_submit_transfer(t);
			m_requested_sampleno += chunk_size;
			return true;
		}
		return false;
	}
	
	void handle_in_transfer(libusb_transfer* t) {
		std::cerr << "handle_in_transfer " << m_in_sampleno << std::endl;
		
		auto buf = (uint16_t*) t->buffer;
		for (size_t i = 0; i < chunk_size; i++) {
			m_dest_buf_v[m_in_sampleno  ] = be16toh(buf[i]);
			m_dest_buf_i[m_in_sampleno++] = be16toh(buf[i+chunk_size]);
		}
		
		if (m_in_sampleno >= m_sample_count) {
			m_completion.notify_all();
		}
	}
	
	void wait() {
		std::unique_lock<std::mutex> lk(m_state);
		m_completion.wait(lk, [&]{ return m_in_sampleno >= m_sample_count; });
	}
	
	libusb_device_handle* const m_usb;
	Transfers m_in_transfers;
	Transfers m_out_transfers;
	
	std::mutex m_state;
	std::condition_variable m_completion;
	
	uint64_t m_sample_rate;
	uint64_t m_sample_count;
	
	// State owned by USB thread
	uint16_t m_requested_sampleno;
	uint64_t m_in_sampleno;
	uint64_t m_out_sampleno;
	
	uint16_t* m_src_buf;
	uint16_t* m_dest_buf_v;
	uint16_t* m_dest_buf_i;
};

/// Runs in USB thread
extern "C" void LIBUSB_CALL in_completion(libusb_transfer *t){
	if (!t->user_data){
		libusb_free_transfer(t); // user_data was zeroed out when device was deleted
		return;
	}

	HeliumDevice *dev = (HeliumDevice *) t->user_data;
	std::lock_guard<std::mutex> lock(dev->m_state);
	
	if (t->status == LIBUSB_TRANSFER_COMPLETED){
		dev->handle_in_transfer(t);
		dev->submit_in_transfer(t);
	}else{
		std::cerr << "ITransfer error "<< t->status << " " << t << std::endl;
		//TODO: notify main thread of error
	}
}

/// Runs in USB thread
extern "C" void LIBUSB_CALL out_completion(libusb_transfer *t){
	if (!t->user_data) {
		libusb_free_transfer(t); // user_data was zeroed out when device was deleted
		return;
	}

	HeliumDevice *dev = (HeliumDevice *) t->user_data;
	std::lock_guard<std::mutex> lock(dev->m_state);
	
	if (t->status == LIBUSB_TRANSFER_COMPLETED){
		dev->submit_out_transfer(t);
	}else{
		std::cerr << "OTransfer error "<< t->status << " " << t << std::endl;
	}
}



int main()
{
	if (libusb_init(NULL) < 0) {
		std::cerr << "Could not init libusb" << std::endl;
		abort();
	}
	
	libusb_set_debug(NULL, 2);
	
	// Dedicated thread for handling soft-real-time USB events
	std::thread usb_thread([]() {
		while(1) libusb_handle_events(NULL);
	});
	
	libusb_device_handle* handle = libusb_open_device_with_vid_pid(NULL, 0x0456, 0xCEE2);
	
	if (!handle) {
		std::cerr << "Device not found" << std::endl;
	}
	
	const size_t len = (1<<14);
	uint16_t out[len];
	for (size_t i=0; i<len; i++) {
		out[i] = (1<<15) + uint16_t(sin(M_PI*2.0*double(i)/double((1<<8)-1))*double((1<<14)-1));
	}
	uint16_t in_v[len];
	uint16_t in_i[len];
		
	HeliumDevice dev(handle);
	
	dev.m_src_buf = out;
	dev.m_dest_buf_v = in_v;
	dev.m_dest_buf_i = in_i;
	
	dev.claim();
	dev.config_sync(0, len);
	dev.start();
	dev.wait();
	dev.stop();	
	dev.release();
	
	for (size_t i=0; i<len; i++) {
		std::cout << out[i] << ", " << in_v[i] << ", " << in_i[i] << std::endl;
	}

	exit(0); //TODO: stop libusb properly
	usb_thread.join();
}
