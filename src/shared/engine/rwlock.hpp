#ifndef RWLOCK_HPP_
#define RWLOCK_HPP_

#ifdef _MSC_VER
#include <windows.h>
#else
#include <pthread.h>
#endif

class ReadWriteLock {
public:
	ReadWriteLock();
	~ReadWriteLock();
	
	void lockRead();
	void lockWrite();
	
	// returns true iff the lock was successfully taken
	bool tryLockRead();
	bool tryLockWrite();

	void unlockRead();
	void unlockWrite();

private:
#ifdef _MSC_VER
	SRWLOCK _lock;
#else
	pthread_rwlock_t _lock;
#endif
};

#endif // RWLOCK_HPP_
