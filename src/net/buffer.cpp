/*
 * buffer.cpp
 *
 *  Created on: 24.09.2014
 *      Author: lars
 */

#include "buffer.hpp"

#include <cstring>

Buffer::~Buffer() {
	if (_data) {
		delete[] _data;
	}
}

Buffer::Buffer() :
	_capacity(0), _data(nullptr), _whead(_data), _rhead(_data)
{
	// nothing
}

Buffer::Buffer(size_t capacity) :
	_capacity(capacity), _data(new char[capacity]),
	_whead(_data), _rhead(_data)
{
	// nothing
}

void Buffer::clear() {
	_whead = _data;
	_rhead = _data;
}

void Buffer::resize(size_t capacity) {
	if (_data) {
		delete[] _data;
	}
	_capacity = capacity;
	_data = new char[capacity];
	_whead = _data;
	_rhead = _data;
}

void Buffer::write(const char *data, size_t len) {
	memcpy(_whead, data, len);
	_whead += len;
}

void Buffer::put(char data) {
	*_whead++ = data;
}

void Buffer::read(char *data, size_t len) const {
	memcpy(data, _rhead, len);
	_rhead += len;
}

Buffer &operator << (Buffer &lhs, const Buffer &rhs) {
	if (lhs.wSize() > rhs.rSize()) {
		lhs.write(rhs.rBegin(), rhs.rSize());
		rhs.rSeekRel(rhs.rSize());
	}
	return lhs;
}

Buffer &operator << (Buffer &lhs, const char *string) {
	while (lhs.wSize() > 0 && *string != '\0') {
		lhs.put(*string++);
	}
	return lhs;
}

