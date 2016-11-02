#include "wiimote_btstack.h"
#include "Windows.h"

extern "C" int main(int argc, char** argv) {
	wiimote_btstack_init();
	while (1) {
		Sleep(1000);
	}
	return 0;
}