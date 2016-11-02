#include "Windows.h"
#include "wiimote_btstack.hpp"
#include "Pipes.hpp"

class WiimotePipe::Impl {
public:
	Impl(int i){
		index = i;
		create(index);
	};
	~Impl(){
		destroy();
	};

	wiimote_report read() {
		uint8_t buffer[32];
		DWORD b, r;
		wiimote_report ret;

		if (!ReadFile(pipe, buffer, 32, &b, &overlap)) {
			b = GetLastError();

			//printf("Error: %ld\n", b);

			if (b == ERROR_HANDLE_EOF || b == ERROR_PIPE_LISTENING) {
				return ret;
			}

			if (b == ERROR_BROKEN_PIPE) {
				//reconnect();
				return ret;
			}

			r = WaitForSingleObject(overlap.hEvent, 10);
			if (r == WAIT_TIMEOUT) {
				CancelIo(pipe);
				ResetEvent(overlap.hEvent);
				return ret;
			}
			else if (r == WAIT_FAILED) {
				return ret;
			}

			if (!GetOverlappedResult(pipe, &overlap, &b, 0)) {
				return ret;
			}
		}
		ResetEvent(overlap.hEvent);

		ret = wiimote_report(&buffer[0], &buffer[b]);
		return ret;
	}

	void write(wiimote_report data) {
		DWORD bytes;
		if (data.size()) {
			WriteFile(pipe, &data[0], data.size(), &bytes, &overlap);
		}
	}

private:
	HANDLE pipe;
	OVERLAPPED overlap;
	int index;

	void create(int index) {
		char buffer[256];
		sprintf_s(buffer, "\\\\.\\pipe\\wiimote%d", index);
		pipe = CreateNamedPipe(buffer,
			PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
			PIPE_UNLIMITED_INSTANCES,
			2046, 2046, 0, NULL);
		if (pipe == INVALID_HANDLE_VALUE) {
			throw std::bad_alloc();
		}
		overlap.hEvent = CreateEvent(NULL, 1, 1, "");
		overlap.Offset = 0;
		overlap.OffsetHigh = 0;
	}
	void connect() {
		ConnectNamedPipe(pipe, &overlap);
	}
	void disconnect() {
		DisconnectNamedPipe(pipe);
	}
	void reconnect() {
		disconnect();
		connect();
	}
	void destroy() {
		CloseHandle(pipe);
	}

};


WiimotePipe::WiimotePipe(int i) {
	index = i;
	pimpl = new Impl(index);
};

WiimotePipe::~WiimotePipe(){
	delete pimpl;
};

wiimote_report WiimotePipe::read(){
	return pimpl->read();
};

void WiimotePipe::write(wiimote_report data) {
	pimpl->write(data);
};