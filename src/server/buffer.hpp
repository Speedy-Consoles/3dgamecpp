/*
 * buffer.hpp
 *
 *  Created on: 24.09.2014
 *      Author: lars
 */

#ifndef BUFFER_HPP_
#define BUFFER_HPP_

#include "std_types.hpp"

class Buffer {
public:
	~Buffer();
	Buffer();
	Buffer(size_t capacity);

	void clear();
	void resize(size_t);

	char *data() { return _data; };
	size_t capacity() const { return _capacity; }

	// writing
	char *wBegin() { return _whead; }
	const char *wEnd() const { return _data + _capacity; }
	size_t wSize() const { return _capacity - (_data - _whead); }

	void wSeekRel(ptrdiff_t diff) { _whead += diff; }
	void wSeek(size_t pos) { _whead = _data + pos; }

	void write(const char *data, size_t len);

	// reading
	const char *rBegin() const { return _rhead; }
	const char *rEnd() const { return _whead; }
	size_t rSize() const { return _whead - _rhead; }

	void rSeekRel(ptrdiff_t diff) { _rhead += diff; }
	void rSeek(size_t pos) { _rhead = _data + pos; }

	void read(char *data, size_t len);


private:
	size_t _capacity = 0;
	char *_data = nullptr;
	char *_whead = nullptr;
	char *_rhead = nullptr;
};

template <typename T>
Buffer &operator << (Buffer &buffer, const T &t) {
	auto ptr = reinterpret_cast<const char *>(&t);
	buffer.write(ptr, sizeof (T));
	return buffer;
}

template <typename T>
Buffer &operator >> (Buffer &buffer, T &t) {
	auto ptr = reinterpret_cast<char *>(&t);
	buffer.read(ptr, sizeof (T));
	return buffer;
}

#endif // BUFFER_HPP_
