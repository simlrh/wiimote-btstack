#pragma once

#include <thread>

#include "wiimote_btstack.hpp"
#include "Wiimote.hpp"
#include "CommandQueue.hpp"

// Unofficial wiimotes only respond to limited inquiry access code
#define HCI_INQUIRY_LIMITED	0x9E8B00L
#define INQUIRY_INTERVAL	5

class WiimoteBTStack {
public:
	static std::shared_ptr<WiimoteBTStack> get();

	WiimoteBTStack();
	~WiimoteBTStack();
	
	bool isReady();

	void discover();

	void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	void handle_inquiry_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	void handle_name_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

	static void hci_packet_callback(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

	std::vector<std::shared_ptr<Wiimote>> wiimotes;
private:
	std::shared_ptr<CommandQueue> commandQueue;

	typedef struct btstack_thread_data {
		std::mutex mutex;
		bool end = false;
		bool ready;
	} btstack_thread_data;
	btstack_thread_data thread_data;
	std::thread *btstack_thread;
	static void btstack_thread_function();

	struct device {
		bd_addr_t  address;
		uint16_t   clockOffset;
		uint32_t   classOfDevice;
		uint8_t    pageScanRepetitionMode;
		uint8_t    rssi;
	};
	std::vector<device> devices;
	int getDeviceIndexForAddress(bd_addr_t addr);

	int getWiimoteIndexForAddress(bd_addr_t addr);
	void addWiimote(bd_addr_t addr, bool pair = true);
};

