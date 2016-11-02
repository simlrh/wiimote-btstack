#pragma once

#include "wiimote_btstack.hpp"
#include "CommandQueue.hpp"

typedef std::vector<uint8_t> wiimote_report;
typedef struct wiimote_address {
	uint8_t address[BD_ADDR_LEN];
	uint8_t padding[10];

} wiimote_address;

class Wiimote {
public:
	Wiimote(std::shared_ptr<CommandQueue> queue, bd_addr_t addr, bool pair);
	~Wiimote();

	uint16_t channel = -1;
	wiimote_address address;
	bool ready = false;

	wiimote_report read();
	void write(wiimote_report report);
	void send_l2cap();
	void receive_l2cap(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	void handle_hci_connection(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	void handle_pin_code_request(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	void l2cap_connect(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
	void disconnect();

	static void outgoing_packet_handler(btstack_timer_source_t *ts);
	static void incoming_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

	static void hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
private:
	std::queue<wiimote_report> input;
	std::queue<wiimote_report> output;
	std::mutex input_mutex;
	std::mutex output_mutex;

	std::shared_ptr<CommandQueue> commandQueue;
	btstack_timer_source_t timer;
	btstack_packet_callback_registration_t hci_event_callback_registration;
};