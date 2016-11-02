#include "wiimote_btstack.hpp"

class WiimotePipe {
public:
	WiimotePipe(int i);
	~WiimotePipe();

	wiimote_report read();
	void write(wiimote_report data);
private:
	int index;
	class Impl;
	Impl* pimpl;
};
