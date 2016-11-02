#include "BTStack.hpp"
#define inline __inline
#include "btstack_run_loop_posix.h"
#include "btstack_link_key_db_fs.h"
#undef inline

std::shared_ptr<WiimoteBTStack> WiimoteBTStack::get()
{
	static std::shared_ptr<WiimoteBTStack> wiimoteBTStack = std::shared_ptr<WiimoteBTStack>(new WiimoteBTStack());
	return wiimoteBTStack;
}

WiimoteBTStack::WiimoteBTStack() {
	commandQueue = std::make_shared<CommandQueue>();
	commandQueue->addCommand(
		[](void) -> bool{
			l2cap_register_service(WiimoteBTStack::hci_packet_callback, WIIMOTE_CONTROL_PSM, WIIMOTE_MTU, LEVEL_0);
			return true;
		}
	);	
	btstack_thread = new std::thread(WiimoteBTStack::btstack_thread_function);
}

WiimoteBTStack::~WiimoteBTStack() {
	for (int i = 0; i < wiimotes.size(); i++) {
		wiimotes[i]->disconnect();
	}
	thread_data.mutex.lock();
	thread_data.end = true;
	thread_data.mutex.unlock();
	btstack_thread->join();
}

bool WiimoteBTStack::isReady()
{
	std::lock_guard<std::mutex> guard(thread_data.mutex);
	return thread_data.ready;
}

void WiimoteBTStack::discover()
{
	commandQueue->addCommand(
		[](void) -> bool{
			printf("Discovering Wiimotes..\n");
			hci_send_cmd(&hci_inquiry, HCI_INQUIRY_LIMITED, INQUIRY_INTERVAL, 0);
			return true;
		}
	);
}

int WiimoteBTStack::getDeviceIndexForAddress(bd_addr_t addr){
	int j;
	for (j = 0; j < devices.size(); j++){
		if (bd_addr_cmp(addr, devices[j].address) == 0){
			return j;
		}
	}
	return -1;
}

void WiimoteBTStack::handle_inquiry_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	bd_addr_t addr;
	int i;
	int numResponses;
	int index;

	numResponses = hci_event_inquiry_result_get_num_responses(packet);
	for (i = 0; i < numResponses; i++){
		reverse_bd_addr(&packet[3 + i * 6], addr);
		index = getDeviceIndexForAddress(addr);

		if (index >= 0) continue;   // already in our list

		struct device dev;

		memcpy(dev.address, addr, 6);
		dev.pageScanRepetitionMode = packet[3 + numResponses*(6) + i * 1];

			dev.classOfDevice = little_endian_read_24(packet, 3 + numResponses*(6 + 1 + 1 + 1) + i * 3);
			dev.clockOffset = little_endian_read_16(packet, 3 + numResponses*(6 + 1 + 1 + 1 + 3) + i * 2) & 0x7fff;
			dev.rssi = 0;

		printf("Device found: %s with COD: 0x%06x, pageScan %d, clock offset 0x%04x\n", bd_addr_to_str(addr),
			(unsigned int)dev.classOfDevice, dev.pageScanRepetitionMode,
			dev.clockOffset);

		devices.push_back(dev);

		commandQueue->addCommand(
			[=](void) -> bool {
				hci_send_cmd(&hci_remote_name_request, dev.address,
					dev.pageScanRepetitionMode, 0, dev.clockOffset | 0x8000);
				return true;
			}
		);
	}
}

void WiimoteBTStack::handle_name_result(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
	int index;
	bd_addr_t addr;

	reverse_bd_addr(&packet[3], addr);
	index = getDeviceIndexForAddress(addr);
	if (index >= 0) {
		if (packet[2] == 0) {
			if (!strcmp((char *)&packet[9], "Nintendo RVL-CNT-01")) {
				printf("Found device: '%s'\n", &packet[9]);
				addWiimote(addr);
			}
		}
	}
}

void WiimoteBTStack::addWiimote(bd_addr_t addr, bool pair)
{
	wiimotes.push_back(std::make_shared<Wiimote>(commandQueue, addr, pair));
}

void WiimoteBTStack::hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{

	if (packet_type != HCI_EVENT_PACKET) {
		printf("Packet type: %x", packet_type);
		return;
	}

	uint8_t event = hci_event_packet_get_type(packet);

	bd_addr_t addr;
	wiimote_address address;
	uint8_t status;

	switch (event){
	case BTSTACK_EVENT_STATE:
		if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING){
			thread_data.mutex.lock();
			thread_data.ready = true;
			commandQueue->processCommand();
			thread_data.mutex.unlock();
		}
		break;
	case HCI_EVENT_INQUIRY_RESULT:
		handle_inquiry_result(packet_type, channel, packet, size);
		break;

	case HCI_EVENT_INQUIRY_COMPLETE:
		printf("Discovery complete.\n");
		break;

	case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
		handle_name_result(packet_type, channel, packet, size);
		break;

	case HCI_EVENT_CONNECTION_REQUEST:
		hci_event_connection_request_get_bd_addr(packet, address.address);

		commandQueue->addCommand(
			[=](void) -> bool {
				hci_send_cmd(&hci_accept_connection_request, address.address, HCI_ROLE_MASTER);
				return true;
			}
		);
		break;
		
	case HCI_EVENT_CONNECTION_COMPLETE:
		hci_event_connection_complete_get_bd_addr(packet, addr);
		addWiimote(addr, false);
		break;

	case HCI_EVENT_DISCONNECTION_COMPLETE:
		status = hci_event_disconnection_complete_get_reason(packet);
		printf("Disconnection reason %x\n", status);
	default:
		break;
	}
}

void WiimoteBTStack::hci_packet_callback(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){
	WiimoteBTStack::get()->hci_event_handler(packet_type, channel, packet, size);
}

void WiimoteBTStack::btstack_thread_function() {
	btstack_memory_init();
	btstack_run_loop_init(btstack_run_loop_posix_get_instance());

	char pklg_path[100];
	strcpy(pklg_path, "hci_dump");
	strcat(pklg_path, ".pklg");
	printf("Packet Log: %s\n", pklg_path);
	hci_dump_open(pklg_path, HCI_DUMP_PACKETLOGGER);

	// init HCI
	hci_init(hci_transport_usb_instance(), NULL);

#ifdef ENABLE_CLASSIC
	hci_set_link_key_db(btstack_link_key_db_fs_instance());
#endif    

	// inform about BTstack state
	btstack_packet_callback_registration_t hci_event_callback_registration;
	hci_event_callback_registration.callback = WiimoteBTStack::hci_packet_callback;
	hci_add_event_handler(&hci_event_callback_registration);

	hci_set_inquiry_mode(INQUIRY_MODE_STANDARD);


	l2cap_init();

	// turn on!
	hci_power_control(HCI_POWER_ON);

	// go
	btstack_run_loop_execute();

#ifndef _WIN32
	// reset anyway
	btstack_stdin_reset();
#endif

	hci_power_control(HCI_POWER_OFF);
	hci_close();
}