#include "Wiimote.hpp"

#include <map>

namespace {
	struct wiimote_address_compare {
		bool operator()(wiimote_address const& a, wiimote_address const& b) {
			for (int i = 0; i < BD_ADDR_LEN; i++) {
				if (a.address[i] != b.address[i]) {
					return a.address[i] < b.address[i];
				}
			}
			return 0;
		}
	};

	// Private list of wiimotes to check against L2CAP channel during contextless callbacks
	std::map<wiimote_address, Wiimote*, wiimote_address_compare> wiimotes;
	std::map<uint16_t, wiimote_address> addresses;

	Wiimote* getWiimoteByChannel(uint16_t channel) {
		for (auto wiimote : wiimotes) {
			if (wiimote.second->channel == channel) {
				return wiimote.second;
			}
		}
		return NULL;
	}
}

void Wiimote::hci_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
	if (packet_type != HCI_EVENT_PACKET) return;

	uint8_t event = hci_event_packet_get_type(packet);

	wiimote_address address;
	uint8_t status;
	uint16_t connection_handle;

	switch (event){
	case HCI_EVENT_CONNECTION_COMPLETE:
		printf("HCI Connected\n");
		hci_event_connection_complete_get_bd_addr(packet, address.address);
		connection_handle = hci_event_connection_complete_get_connection_handle(packet);
		addresses[connection_handle] = address;
		wiimotes[address]->handle_hci_connection(packet_type, channel, packet, size);
		break;
	case HCI_EVENT_PIN_CODE_REQUEST:
		printf("Pin code requested\n");
		hci_event_pin_code_request_get_bd_addr(packet, address.address);
		wiimotes[address]->handle_pin_code_request(packet_type, channel, packet, size);
		break;
	case HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT:
		status = hci_event_authentication_complete_event_get_status(packet);
		printf("Authentication complete %d\n", status);
		connection_handle = hci_event_authentication_complete_event_get_connection_handle(packet);
		wiimotes[addresses[connection_handle]]->l2cap_connect(packet_type, channel, packet, size);
	default:
		break;
	}
}

// BTStack L2CAP packet handler
void Wiimote::incoming_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
	Wiimote* wiimote = getWiimoteByChannel(channel);
	if (wiimote) wiimote->receive_l2cap(packet_type, channel, packet, size);
}

// BTStack timer
void Wiimote::outgoing_packet_handler(btstack_timer_source_t *ts){
	Wiimote* wiimote = reinterpret_cast<Wiimote*>(ts->context);
	if (wiimote) wiimote->send_l2cap();
	btstack_run_loop_set_timer(ts, 10);
	btstack_run_loop_add_timer(ts);
}

void Wiimote::handle_hci_connection(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	uint16_t connection_handle = hci_event_connection_complete_get_connection_handle(packet);
	commandQueue->addCommand(
		[=](void) -> bool {
			hci_send_cmd(&hci_authentication_requested, connection_handle);
			return true;
		}
	);
}


void Wiimote::handle_pin_code_request(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	wiimote_address address, pin_backwards, pin_forwards;
	hci_event_pin_code_request_get_bd_addr(packet, address.address);
	gap_local_bd_addr(pin_backwards.address);
	//hci_event_pin_code_request_get_bd_addr(packet, pin.address);
	reverse_bd_addr(pin_backwards.address, pin_forwards.address);
	commandQueue->addCommand(
		[=](void) -> bool {
		hci_send_cmd(&hci_pin_code_request_reply, address.address, 6, pin_forwards.address);
		return true;
	}
	);
}

Wiimote::Wiimote(std::shared_ptr<CommandQueue> queue, bd_addr_t addr, bool pair) {
	memcpy(&address, addr, BD_ADDR_LEN);

	timer.process = Wiimote::outgoing_packet_handler;
	btstack_run_loop_set_timer_context(&timer, this);
	btstack_run_loop_set_timer(&timer, 10);

	commandQueue = queue;

	if (pair) {
		hci_event_callback_registration.callback = Wiimote::hci_packet_handler;

		commandQueue->addCommand(
			[&](void) -> bool {
			hci_add_event_handler(&hci_event_callback_registration);
			return true;
		}
		);
		commandQueue->addCommand(
			[=](void) -> bool {
			hci_send_cmd(&hci_create_connection, &address, hci_usable_acl_packet_types(), 0, 0, 0, 1);
			return true;
		}
		);

	}
	else {
		l2cap_connect(0, 0, 0, 0);
	}

	wiimotes[address] = this;
}

void Wiimote::l2cap_connect(uint8_t packet_type, uint16_t c, uint8_t *packet, uint16_t size)
{
	commandQueue->addCommand(
		[&](void) -> bool {
			l2cap_create_channel(Wiimote::incoming_packet_handler, address.address, WIIMOTE_DATA_PSM, WIIMOTE_MTU, &channel);
			return true;
		}
	);
}

void Wiimote::disconnect()
{
	commandQueue->addCommand(
		[&](void) -> bool {
			l2cap_disconnect(channel, 0);
			return true;
		}
	);
}


Wiimote::~Wiimote() {
	disconnect();
	wiimotes.erase(address);
}

void Wiimote::send_l2cap()
{
	if (ready) {
		output_mutex.lock();
		while (output.size() && l2cap_can_send_packet_now(channel)) {
			l2cap_send(channel, &output.front()[0], output.front().size());
			output.pop();
		}
		output_mutex.unlock();
	}
}

void Wiimote::receive_l2cap(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	uint8_t event = hci_event_packet_get_type(packet);

	switch (event) {
	case L2CAP_EVENT_CHANNEL_OPENED:
		btstack_run_loop_add_timer(&timer);
		l2cap_request_can_send_now_event(channel);
		break;
	case L2CAP_EVENT_CHANNEL_CLOSED:
		ready = false;
		break;
	case L2CAP_EVENT_CAN_SEND_NOW:
		ready = true;
		break;
	case GATT_EVENT_SERVICE_QUERY_RESULT:
		input_mutex.lock();
		input.push(std::vector<uint8_t>(packet, packet + size));
		input_mutex.unlock();
		break;
	default:
		log_info("Unknwon L2CAP packet: %X", event);
		printf_hexdump(packet, size);
		break;
	}
}

wiimote_report Wiimote::read(){
	input_mutex.lock();
	wiimote_report report;
	if (input.size()) {
		report = input.front();
		report.erase(report.begin());
		input.pop();
	}
	input_mutex.unlock();
	return report;
}

void Wiimote::write(wiimote_report report){
	report.insert(report.begin(), 0xA2);
	output_mutex.lock();
	output.push(report);
	output_mutex.unlock();
}