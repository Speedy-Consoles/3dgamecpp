#ifndef BUFFER_HPP_
#define BUFFER_HPP_

#include "std_types.hpp"

class Buffer {
public:
	~Buffer();
	Buffer();
	Buffer(size_t capacity);

	//moving
	Buffer(Buffer &&that);
	Buffer &operator = (Buffer &&rhs);

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
	void put(char data);

	// reading
	const char *rBegin() const { return _rhead; }
	const char *rEnd() const { return _whead; }
	size_t rSize() const { return _whead - _rhead; }

	void rSeekRel(ptrdiff_t diff) const { _rhead += diff; }
	void rSeek(size_t pos) const { _rhead = _data + pos; }

	void read(char *data, size_t len) const;

private:
	size_t _capacity = 0;
	char *_data = nullptr;
	char *_whead = nullptr;
	mutable char *_rhead = nullptr;
};

Buffer &operator << (Buffer &lhs, const Buffer &rhs);
Buffer &operator << (Buffer &lhs, const char *string);

template <typename T>
Buffer &operator << (Buffer &lhs, const T &rhs) {
	if (lhs.wSize() >= sizeof (T))
		lhs.write(reinterpret_cast<const char *>(&rhs), sizeof (T));
	return lhs;
}

template <typename T>
const Buffer &operator >> (const Buffer &lhs, T &rhs) {
	if (lhs.rSize() >= sizeof (T))
		lhs.read(reinterpret_cast<char *>(&rhs), sizeof (T));
	return lhs;
}

#endif // BUFFER_HPP_
