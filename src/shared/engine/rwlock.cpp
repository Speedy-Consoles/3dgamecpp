#include "rwlock.hpp"

#ifdef _MSC_VER

#include <Synchapi.h>

ReadWriteLock::ReadWriteLock() {
	InitializeSRWLock(&_lock);
}

ReadWriteLock::~ReadWriteLock() {
	// nothing
}
	
void ReadWriteLock::lockRead() {
	AcquireSRWLockShared(&_lock);
}

void ReadWriteLock::lockWrite() {
	AcquireSRWLockExclusive(&_lock);
}
	
bool ReadWriteLock::tryLockRead() {
	return TryAcquireSRWLockShared(&_lock) != 0;
}

bool ReadWriteLock::tryLockWrite() {
	return TryAcquireSRWLockExclusive(&_lock) != 0;
}

void ReadWriteLock::unlockRead() {
	ReleaseSRWLockShared(&_lock);
}

void ReadWriteLock::unlockWrite() {
	ReleaseSRWLockExclusive(&_lock);
}

#else

ReadWriteLock::ReadWriteLock() {
	pthread_rwlock_init(&_lock);
}

ReadWriteLock::~ReadWriteLock() {
	pthread_rwlock_destroy(&_lock);
}
	
void ReadWriteLock::lockRead() {
	pthread_rwlock_rdlock(&_lock);
}

void ReadWriteLock::lockWrite() {
	pthread_rwlock_wrlock(&_lock);
}
	
bool ReadWriteLock::tryLockRead() {
	return pthread_rwlock_tryrdlock(&_lock) == 0;
}

bool ReadWriteLock::tryLockWrite() {
	return pthread_rwlock_trywrlock(&_lock) == 0;
}

void ReadWriteLock::unlockRead() {
	pthread_rwlock_unlock(&_lock);
}

void ReadWriteLock::unlockWrite() {
	pthread_rwlock_unlock(&_lock);
}

#endif
