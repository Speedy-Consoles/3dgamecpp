#ifndef QUEUE_HPP
#define QUEUE_HPP

#include <atomic>
#include <cstddef>

template <class T>
class ProducerQueue {
public:
	ProducerQueue(size_t size);
	~ProducerQueue();

	bool push(const T &);
	bool push(T &&);

	bool pop(T &);

private:
	size_t _size;
	T *_data;
	std::atomic<size_t> _head;
	std::atomic<size_t> _tail;
};

template <class T>
ProducerQueue<T>::ProducerQueue(size_t size) :
	_size(size), _data(new T[size]),
	_head(0), _tail(0)
{
	// nothing
}

template <class T>
ProducerQueue<T>::~ProducerQueue() {
	delete[] _data;
}

template <class T>
bool ProducerQueue<T>::push(const T &object) {
	size_t head = _head.load(std::memory_order_relaxed);
	size_t next = (head + 1) % _size;
	if (next == _tail.load(std::memory_order_acquire)) {
		return false;
	}
	_data[head] = object;
	_head.store(next, std::memory_order_release);
	return true;
}

template <class T>
bool ProducerQueue<T>::push(T &&object) {
	size_t head = _head.load(std::memory_order_relaxed);
	size_t next = (head + 1) % _size;
	if (next == _tail.load(std::memory_order_acquire)) {
		return false;
	}
	_data[head] = std::move(object);
	_head.store(next, std::memory_order_release);
	return true;
}

template <class T>
bool ProducerQueue<T>::pop(T &object) {
	size_t tail = _tail.load(std::memory_order_relaxed);
	if (tail == _head.load(std::memory_order_acquire)) {
		return false;
	}
	object = _data[tail];
	_tail.store((tail + 1) % _size, std::memory_order_release);
	return true;
}

#endif // QUEUE_HPP
