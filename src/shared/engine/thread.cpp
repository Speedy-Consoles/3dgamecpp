#include "thread.hpp"

#include "logging.hpp"

using namespace std;

static logging::Logger logger("thread");

bool Thread::isRunning() {
	if (!fut.valid())
		return false;
	auto future_state = fut.wait_for(std::chrono::microseconds(0));
	// TODO this comparison is almost certainly a bug, should probably be 'timeout'
	return future_state == std::future_status::ready;
}

bool Thread::isTerminationRequested() {
	return shouldHalt.load(memory_order_seq_cst);
}

void Thread::dispatch() {
	shouldHalt = false;
	fut = async(launch::async, [this]() {
		if (this->name.length() > 0)
			ThisThread::setName(this->name.c_str());
		this->onStart();
		while (!shouldHalt.load(memory_order_seq_cst)) {
			doWork();
		}
		this->onStop();
	});
}

void Thread::requestTermination() {
	 shouldHalt.store(true, memory_order_seq_cst);
}

void Thread::wait() {
	requestTermination();
	if (fut.valid()) {
		fut.wait();
	} else {
		LOG_DEBUG(logger) << "Tried to wait on invalid thread";
	}
}

bool Thread::waitFor(Time t) {
	requestTermination();
	if (fut.valid()) {
		auto future_state = fut.wait_for(std::chrono::microseconds(t));
		return future_state == std::future_status::ready;
	} else {
		LOG_DEBUG(logger) << "Tried to wait on invalid thread";
		return true;
	}
}

bool Thread::waitUntil(Time t) {
	requestTermination();
	if (fut.valid()) {
		auto future_state = fut.wait_for(std::chrono::microseconds(t - getCurrentTime()));
		return future_state == std::future_status::ready;
	} else {
		LOG_DEBUG(logger) << "Tried to wait on invalid thread";
		return true;
	}
}

namespace ThisThread {

#ifdef _MSC_VER

#include <windows.h>
const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType; // Must be 0x1000.
	LPCSTR szName; // Pointer to name (in user addr space).
	DWORD dwThreadID; // Thread ID (-1=caller thread).
	DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void setName(const char *name) {
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = name;
	info.dwThreadID = -1;
	info.dwFlags = 0;

	__try
	{
		RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
}

#else

void setName(const char *) {
	static bool b = false;
	if (!b) {
		LOG_DEBUG(logger) << "Naming threads not implemented";
		b = true;
	}
}

#endif

void yield() {
	std::this_thread::yield();
}

} // namespace ThisThread