#ifndef MUTEX_HPP
#define MUTEX_HPP

#ifdef _MSC_VER
#define NOMINMAX
#include <WinSock2.h>
#include <Windows.h>
#else
#include <pthread.h>
#endif

class Mutex {
public:
	Mutex();
	~Mutex();

	void lock();
	// returns true iff the lock was successfully taken
	bool tryLock();
	void unlock();

private:
#ifdef _MSC_VER
	CRITICAL_SECTION _mutex;
#else
	pthread_mutex_t _mutex;
#endif
};

#endif // MUTEX_HPP
