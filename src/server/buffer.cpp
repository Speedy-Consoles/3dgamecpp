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

void Buffer::read(char *data, size_t len) {
	memcpy(data, _rhead, len);
	_rhead += len;
}


