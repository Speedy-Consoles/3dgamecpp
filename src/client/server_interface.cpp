#include "server_interface.hpp"

using namespace std;

void ServerInterface::dispatch() {
	shouldHalt = false;
	fut = async(launch::async, [this]() { run(); });
}

void ServerInterface::requestTermination() {
	 shouldHalt.store(true, memory_order_relaxed);
}

void ServerInterface::wait() {
	requestTermination();
	if (fut.valid())
		fut.get();
}
