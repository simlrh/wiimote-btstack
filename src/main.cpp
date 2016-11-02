#include "wiimote_btstack.hpp"
#include "Pipes.hpp"
#include "BTStack.hpp"
#include "Windows.h"
#include <conio.h>

#include <vector>
#include <memory>

std::vector<std::unique_ptr<WiimotePipe>> wiimote_pipes;

int main(int argc, char** argv) {
	bool set = false;
	std::shared_ptr<WiimoteBTStack> wiimotes = WiimoteBTStack::get();
	wiimote_report report;
	while (!wiimotes->isReady()) {
		Sleep(100);
	}
	while (1) {
		if (_kbhit()) {
			if (_getch() == 13) {
				wiimotes->discover();
			}
		}
		for (unsigned int i = 0; i < wiimotes->wiimotes.size(); i++) {
			std::shared_ptr<Wiimote> wiimote = wiimotes->wiimotes[i];
			if (i == wiimote_pipes.size()) {
				wiimote_pipes.push_back(std::make_unique<WiimotePipe>(i));
			}
			do {
				report = wiimote_pipes[i]->read();
				if (report.size()) {
#ifdef _DEBUG
					printf("Sending to Wiimote %d: ", i);
					for (auto byte : report)
					{
						printf("%x ", byte);
					}
					printf("\n");
#endif
					wiimote->write(report);
				}
			} while (report.size());
			do {
				report = wiimote->read();
				if (report.size()) {
#ifdef _DEBUG
					printf("Received from Wiimote %d: ", i);
					for (auto byte : report)
					{
						printf("%x ", byte);
					}
					printf("\n");
#endif
					wiimote_pipes[i]->write(report);
				}
			} while (report.size());
		}
	}
	return 0;
}