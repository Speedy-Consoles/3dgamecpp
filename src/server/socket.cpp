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

Socket::ErrorCode Socket::receive(
		Buffer *b, endpoint_t *e)
{
	// try to acquire next package synchronously
	switch (receiveNow(b, e)) {
	case OK:
		return OK;
	case WOULD_BLOCK:
		startAsyncReceive(b, e);
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

Socket::ErrorCode Socket::receiveNow(
		Buffer *b, endpoint_t *e)
{
	if (!_recvFuture.valid()) {
		auto buf = asio::buffer(b->wBegin(), b->wSize());
		size_t bytesRead = _socket.receive_from(buf, *e, 0, _error);

		if (!_error) {
			b->wSeekRel(bytesRead);
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

Socket::ErrorCode Socket::receiveFor(
		Buffer *b, endpoint_t *e, uint64 dur)
{
	// try to acquire next package synchronously
	switch (receiveNow(b, e)) {
	case OK:
		return OK;
	case WOULD_BLOCK:
		startAsyncReceive(b, e);
		break;
	case TIMEOUT:
		break;
	default:
		return SYSTEM_ERROR;
	}

	// wait for the specified time
	auto status = _recvFuture.wait_for(std::chrono::microseconds(dur));
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

Socket::ErrorCode Socket::receiveUntil(
		Buffer *b, endpoint_t *e, uint64 time)
{
	time_t now = my::time::get();
	return receiveFor(b, e, time - now);
}

void Socket::startAsyncReceive(Buffer *b, endpoint_t *e) {
	_recvPromise = std::promise<error_t>();
	_recvFuture = _recvPromise.get_future();
	auto buf = asio::buffer(b->wBegin(), b->wSize());
	_socket.async_receive_from(buf, *e, [this, b](const error_t &err, size_t size)
	{
		if (!err) {
			b->wSeekRel(size);
		}
		_recvPromise.set_value(err);
	});
}

Socket::ErrorCode Socket::send(
		const Buffer &b, const endpoint_t &e)
{
	// try to send packet synchronously
	switch (sendNow(b, e)) {
	case OK:
		return OK;
	case WOULD_BLOCK:
		startAsyncSend(b, e);
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

Socket::ErrorCode Socket::sendNow(
		const Buffer &b, const endpoint_t &e)
{
	if (!_sendFuture.valid()) {
		auto buf = asio::buffer(b.rBegin(), b.rSize());
		size_t bytesSent = _socket.send_to(buf, e, 0, _error);

		if (!_error) {
			b.rSeekRel(bytesSent);
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

Socket::ErrorCode Socket::sendFor(
		const Buffer &b, const endpoint_t &e, uint64 duration)
{
	// try to send packet synchronously
	switch (sendNow(b, e)) {
	case OK:
		return OK;
	case WOULD_BLOCK:
		startAsyncSend(b, e);
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

Socket::ErrorCode Socket::sendUntil(
		const Buffer &b, const endpoint_t &e, uint64 time)
{
	time_t now = my::time::get();
	return sendFor(b, e, time - now);
}

void Socket::startAsyncSend(
		const Buffer &b, const endpoint_t &e)
{
	_sendPromise = std::promise<error_t>();
	_sendFuture = _sendPromise.get_future();
	auto buf = asio::buffer(b.rBegin(), b.rSize());
	_socket.async_send_to(buf, e, [this, &b](const error_t &err, size_t size)
	{
		if (!err) {
			b.rSeekRel(size);
		}
		_sendPromise.set_value(err);
	});
}

}} // namespace my::net
