/*
 * socket.cpp
 *
 *  Created on: 24.09.2014
 *      Author: lars
 */

#include "socket.hpp"

#include "io/logging.hpp"

using namespace boost;
using namespace boost::asio::ip;

namespace my { namespace net {

Socket::~Socket() {
	if (_socket.is_open()) {
		_socket.close();
	}
}

Socket::Socket(ios_t &ios) :
	_socket(ios)
{
	// nothing
}

Socket::Socket(ios_t &ios, const endpoint_t &endpoint) :
	_socket(ios, endpoint)
{
	_socket.non_blocking(true, _error);
}

const error_t &Socket::getSystemError() const {
	return _error;
}

void Socket::open() {
	_socket.open(udp::v4());
	_socket.non_blocking(true, _error);
}

void Socket::close() {
	_socket.shutdown(_socket.shutdown_both);
	_socket.close();
}

bool Socket::isOpen() const {
	return _socket.is_open();
}

void Socket::connect(const endpoint_t &endpoint) {
	_socket.connect(endpoint, _error);
}

void Socket::bind(const endpoint_t &endpoint) {
	_socket.bind(endpoint, _error);
}

Socket::ErrorCode Socket::receive(Packet *p) {
	// try to acquire next package synchronously
	switch (receiveNow(p)) {
	case OK:
		return OK;
	case WOULD_BLOCK:
		startAsyncReceive(p);
		break;
	case TIMEOUT:
		break;
	default:
		LOG(ERROR, "While waiting for packages");
		return SYSTEM_ERROR;
	}

	// wait for the specified time
	_error = _recvFuture.get();
	return _error ? SYSTEM_ERROR : OK;
}

Socket::ErrorCode Socket::receiveNow(Packet *p) {
	if (!_recvFuture.valid()) {
		auto buf = asio::buffer(p->buf->wBegin(), p->buf->wSize());
		size_t bytesRead = _socket.receive_from(buf, p->endpoint, 0, _error);

		if (!_error) {
			p->buf->wSeekRel(bytesRead);
			return OK;
		} else if (_error == asio::error::would_block) {
			return WOULD_BLOCK;
		} else {
			LOG(ERROR, "While looking for packages");
			return SYSTEM_ERROR;
		}
	} else {
		return TIMEOUT;
	}
}

Socket::ErrorCode Socket::receiveFor(Packet *p, uint64 duration) {
	// try to acquire next package synchronously
	switch (receiveNow(p)) {
	case OK:
		return OK;
	case WOULD_BLOCK:
		startAsyncReceive(p);
		break;
	case TIMEOUT:
		break;
	default:
		return SYSTEM_ERROR;
	}

	// wait for the specified time
	auto status = _recvFuture.wait_for(std::chrono::microseconds(duration));
	switch (status) {
	case std::future_status::ready:
		_error = _recvFuture.get();
		return _error ? SYSTEM_ERROR : OK;
	case std::future_status::timeout:
		return TIMEOUT;
	default:
		LOG(ERROR, "Deferred future");
		return UNKNOWN_ERROR;
	}
}

Socket::ErrorCode Socket::receiveUntil(Packet *p, uint64 time) {
	time_t now = my::time::get();
	return receiveFor(p, time - now);
}

void Socket::startAsyncReceive(Packet *p) {
	auto buf = asio::buffer(p->buf->wBegin(), p->buf->wSize());

	auto lambda = [this, p](const error_t &error, size_t size) {
		if (!error) {
			p->buf->wSeekRel(size);
		}
		_recvPromise.set_value(error);
	};

	_recvPromise = std::promise<error_t>();
	_recvFuture = _recvPromise.get_future();
	_socket.async_receive_from(buf, p->endpoint, lambda);
}

Socket::ErrorCode Socket::send(const Packet &p) {
	// try to send packet synchronously
	switch (sendNow(p)) {
	case OK:
		return OK;
	case WOULD_BLOCK:
		startAsyncSend(p);
		break;
	case TIMEOUT:
		break;
	default:
		LOG(ERROR, "While trying to send packet");
		return SYSTEM_ERROR;
	}

	// wait for the specified time
	_error = _sendFuture.get();
	return _error ? SYSTEM_ERROR : OK;
}

Socket::ErrorCode Socket::sendNow(const Packet &p) {
	if (!_sendFuture.valid()) {
		auto buf = asio::buffer(p.buf->rBegin(), p.buf->rSize());
		size_t bytesSent = _socket.send_to(buf, p.endpoint, 0, _error);

		if (!_error) {
			p.buf->rSeekRel(bytesSent);
			return OK;
		} else if (_error == asio::error::would_block) {
			return WOULD_BLOCK;
		} else {
			LOG(ERROR, "While sending packets");
			return SYSTEM_ERROR;
		}
	} else {
		return TIMEOUT;
	}
}

Socket::ErrorCode Socket::sendFor(const Packet &p, uint64 duration) {
	// try to send packet synchronously
	switch (sendNow(p)) {
	case OK:
		return OK;
	case WOULD_BLOCK:
		startAsyncSend(p);
		break;
	case TIMEOUT:
		break;
	default:
		return SYSTEM_ERROR;
	}

	// wait for the specified time
	auto status = _sendFuture.wait_for(std::chrono::microseconds(duration));
	switch (status) {
	case std::future_status::ready:
		_error = _sendFuture.get();
		return _error ? SYSTEM_ERROR : OK;
	case std::future_status::timeout:
		return TIMEOUT;
	default:
		LOG(ERROR, "Deferred future");
		return UNKNOWN_ERROR;
	}
}

Socket::ErrorCode Socket::sendUntil(const Packet &p, uint64 time) {
	time_t now = my::time::get();
	return sendFor(p, time - now);
}

void Socket::startAsyncSend(const Packet &p) {
	auto buf = asio::buffer(p.buf->rBegin(), p.buf->rSize());

	auto lambda = [this, &p](const error_t &error, size_t size) {
		if (!error) {
			p.buf->rSeekRel(size);
		}
		_sendPromise.set_value(error);
	};

	_sendPromise = std::promise<error_t>();
	_sendFuture = _sendPromise.get_future();
	_socket.async_send_to(buf, p.endpoint, lambda);
}

}} // namespace my::net

