#include "CommandQueue.hpp"

static void next_command(btstack_timer_source_t *ts){
	CommandQueue *commandQueue = reinterpret_cast<CommandQueue*>(ts->context);
	commandQueue->processCommand();
}

CommandQueue::CommandQueue()
{
}

CommandQueue::~CommandQueue()
{
	btstack_run_loop_remove_timer(&timer);
}

void CommandQueue::addCommand(command c)
{
	mutex.lock();
	commands.push(c);
	mutex.unlock();
}

void CommandQueue::processCommand()
{
	mutex.lock();
	if (commands.size() && hci_can_send_command_packet_now() && commands.front()()){
		commands.pop();
	}
	mutex.unlock();

	timer.process = next_command;
	btstack_run_loop_set_timer(&timer, 10);
	btstack_run_loop_set_timer_context(&timer, this);
	btstack_run_loop_add_timer(&timer);
}