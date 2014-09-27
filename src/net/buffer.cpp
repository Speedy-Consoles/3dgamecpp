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
	_capacity(0), _data(nullptr), _whead(nullptr), _rhead(nullptr)
{
	// nothing
}

Buffer::Buffer(size_t capacity) :
	_capacity(capacity), _data(new char[capacity]),
	_whead(_data), _rhead(_data)
{
	// nothing
}

Buffer::Buffer(Buffer &&that) {
	this->_capacity = that._capacity;
	this->_data = that._data;
	this->_whead = that._whead;
	this->_rhead = that._rhead;

	that._capacity = 0;
	that._data = nullptr;
	that._whead = nullptr;
	that._rhead = nullptr;
}

Buffer &Buffer::operator = (Buffer &&rhs) {
	this->_capacity = rhs._capacity;
	this->_data = rhs._data;
	this->_whead = rhs._whead;
	this->_rhead = rhs._rhead;

	rhs._capacity = 0;
	rhs._data = nullptr;
	rhs._whead = nullptr;
	rhs._rhead = nullptr;

	return *this;
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

