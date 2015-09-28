#ifndef MUTEX_HPP
#define MUTEX_HPP

#ifdef _MSC_VER
#include <windows.h>
#else
#include <pthread.h>
#endif

class Mutex {
public:
	Mutex();
	~Mutex();

	void lock();
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
