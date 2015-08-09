#include "thread.hpp"

using namespace std;

bool Thread::isRunning() {
	if (!fut.valid())
		return false;
	auto future_state = fut.wait_for(std::chrono::microseconds(0));
	return future_state == std::future_status::ready;
}

void Thread::dispatch() {
	shouldHalt = false;
	fut = async(launch::async, [this]() {
		this->onStart();
		while (!shouldHalt.load(memory_order_seq_cst)) {
			doWork();
		}
		this->onStop();
	});
}

void Thread::requestTermination() {
	 shouldHalt.store(true, memory_order_relaxed);
}

void Thread::wait() {
	requestTermination();
	if (fut.valid())
		fut.wait();
}

bool Thread::waitFor(Time t) {
	requestTermination();
	if (fut.valid()) {
		auto future_state = fut.wait_for(std::chrono::microseconds(t));
		return future_state == std::future_status::ready;
	} else {
		return true;
	}
}

bool Thread::waitUntil(Time t) {
	requestTermination();
	if (fut.valid()) {
		auto future_state = fut.wait_for(std::chrono::microseconds(t - getCurrentTime()));
		return future_state == std::future_status::ready;
	} else {
		return true;
	}
}
