#ifndef THREAD_HPP_
#define THREAD_HPP_

#include <future>
#include <atomic>

#include "time.hpp"

class Thread {
	std::future<void> fut;
	std::atomic<bool> shouldHalt;

public:
	Thread() = default;
	virtual ~Thread() = default;
	
	// disallow copy
	Thread(const Thread &) = delete;
	Thread &operator = (const Thread &) = delete;

	// query
	bool isRunning();

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

#endif // THREAD_HPP_
