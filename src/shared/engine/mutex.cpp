#include "mutex.hpp"

#ifdef _MSC_VER

#include <Synchapi.h>

Mutex::Mutex() {
	InitializeCriticalSection(&_mutex);
}

Mutex::~Mutex() {

}

void Mutex::lock() {
	EnterCriticalSection(&_mutex);
}

bool Mutex::tryLock() {
	return TryEnterCriticalSection(&_mutex) == TRUE;
}

void Mutex::unlock() {
	LeaveCriticalSection(&_mutex);
}

#else

Mutex::Mutex() {
	pthread_mutex_init(&_mutex, nullptr);
}

Mutex::~Mutex() {
	pthread_mutex_destroy(&_mutex);
}

void Mutex::lock() {
	pthread_mutex_lock(&_mutex);
}

bool Mutex::tryLock() {
	return pthread_mutex_trylock(&_mutex) == 0;
}

void Mutex::unlock() {
	pthread_mutex_unlock(&_mutex);
}

#endif
