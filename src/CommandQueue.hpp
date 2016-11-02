#pragma once

#include "wiimote_btstack.hpp"

typedef std::function<bool(void)> command;

class CommandQueue {
public:
	CommandQueue();
	~CommandQueue();

	void addCommand(command c);
	void processCommand();
private:
	std::queue<command> commands;
	std::mutex mutex;
	btstack_timer_source_t timer;
};