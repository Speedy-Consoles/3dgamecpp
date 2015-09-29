#ifndef THREAD_HPP_
#define THREAD_HPP_

#include <future>
#include <atomic>

#include "time.hpp"

class Thread {
	std::future<void> fut;
	std::atomic<bool> shouldHalt;
	std::string name;

public:
	Thread() = default;
	Thread(const char *name) : name(name) {};
	virtual ~Thread() = default;
	
	// disallow copy
	Thread(const Thread &) = delete;
	Thread &operator = (const Thread &) = delete;

	// query
	bool isRunning();
	bool isTerminationRequested();

	// operation
	void dispatch();
	virtual void doWork() = 0;
	virtual void onStart() {};
	virtual void onStop() {};

	void requestTermination();
	void wait();
	bool waitFor(Time);
	bool waitUntil(Time);
};

namespace ThisThread {
	void setName(const char *);
	void yield();
}

#endif // THREAD_HPP_
