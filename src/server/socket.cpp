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
		_socket.shutdown(socket_t::shutdown_both);
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
	_ec = _error ? UNKNOWN_ERROR : OK;
}

Socket::ErrorCode Socket::getErrorCode() const {
	return _ec;
}

const error_t &Socket::getSystemError() const {
	return _error;
}

void Socket::open() {
	_socket.open(udp::v4());
	if ((_ec = _error ? UNKNOWN_ERROR : OK))
		return;
	_socket.non_blocking(true, _error);
	_ec = _error ? UNKNOWN_ERROR : OK;
}

void Socket::close() {
	_socket.shutdown(_socket.shutdown_both);
	_socket.close();
}

bool Socket::isOpen() const {
	return _socket.is_open();
}

void Socket::connect(const endpoint_t &endpoint) {
	_socket.connect(endpoint);
}

void Socket::bind(const endpoint_t &endpoint) {
	_socket.bind(endpoint);
}

void Socket::receive(Packet *p) {
	// try to acquire next package synchronously
	if (!_future.valid()) {
		receiveNow(p);
		if (_ec == OK) {
			return;
		} else if (_ec == WOULD_BLOCK) {
			startAsyncReceive(p);
		} else {
			LOG(ERROR, "While waiting for packages");
			return;
		}
	}

	// wait for the specified time
	_future.get();
}

void Socket::receiveNow(Packet *p) {
	auto buf = asio::buffer(p->buf->wBegin(), p->buf->wSize());
	size_t bytesRead = _socket.receive_from(buf, p->endpoint, 0, _error);

	if (!_error) {
		p->buf->wSeekRel(bytesRead);
	} else if (_error == asio::error::would_block) {
		_ec = WOULD_BLOCK;
	} else {
		LOG(ERROR, "While looking for packages");
		_ec = UNKNOWN_ERROR;
	}
}

void Socket::receiveFor(Packet *p, uint64 duration) {
	// try to acquire next package synchronously
	if (!_future.valid()) {
		receiveNow(p);
		if (_ec == OK) {
			return;
		} else if (_ec == WOULD_BLOCK) {
			startAsyncReceive(p);
		} else {
			LOG(ERROR, "While waiting for packages");
			return;
		}
	}

	// wait for the specified time
	auto status = _future.wait_for(std::chrono::microseconds(duration));
	switch (status) {
	case std::future_status::ready:
		_future.get();
		_ec = OK;
		break;
	case std::future_status::timeout:
		_ec = TIMEOUT;
		break;
	default:
		LOG(ERROR, "Deferred future");
		_ec = UNKNOWN_ERROR;
		break;
	}
}

void Socket::receiveUntil(Packet *p, uint64 time) {
	time_t now = my::time::get();
	receiveFor(p, time - now);
}

void Socket::startAsyncReceive(Packet *p) {
	auto buf = asio::buffer(p->buf->wBegin(), p->buf->wSize());

	auto lambda = [this, p](const error_t &error, size_t size) {
		if (!error) {
			p->buf->wSeekRel(size);
			_ec = OK;
		} else if (error == boost::asio::error::message_size) {
			_ec = Socket::BUFFER_TOO_SMALL;
		}
		_error = error;
		_promise.set_value();
	};

	_promise = std::promise<void>();
	_future = _promise.get_future();
	_socket.async_receive_from(buf, p->endpoint, lambda);
}

void Socket::send(const Packet &) {

}

void Socket::sendNow(const Packet &) {

}

void Socket::sendFor(const Packet &, uint64 duration) {

}

void Socket::sendUntil(const Packet &, uint64 time) {

}

}} // namespace my::net

