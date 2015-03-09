/*
 * socket.cpp
 *
 *  Created on: 24.09.2014
 *      Author: lars
 */

#include "socket.hpp"

#include "logging.hpp"

#include <utility>

using namespace boost;
using namespace boost::asio::ip;

Socket::~Socket() {
	if (_socket.is_open()) {
		_socket.close();
	}
}

Socket::Socket(boost::asio::io_service &ios) :
	_socket(ios)
{
	// nothing
}

Socket::Socket(boost::asio::io_service &ios, const Endpoint &endpoint) :
	_socket(ios, endpoint)
{
	_socket.non_blocking(true, _error);
}

const boost::system::error_code &Socket::getSystemError() const {
	return _error;
}

Socket::ErrorCode Socket::open() {
	_socket.open(udp::v4());
	_socket.non_blocking(true, _error);
	return _error ? SYSTEM_ERROR : OK;
}

void Socket::close() {
	_socket.shutdown(_socket.shutdown_both);
	_socket.close();
}

bool Socket::isOpen() const {
	return _socket.is_open();
}

Socket::ErrorCode Socket::connect(const Endpoint &endpoint) {
	_socket.connect(endpoint, _error);
	return _error ? SYSTEM_ERROR : OK;
}

Socket::ErrorCode Socket::bind(const Endpoint &endpoint) {
	_socket.bind(endpoint, _error);
	return _error ? SYSTEM_ERROR : OK;
}

Socket::ErrorCode Socket::receive(Endpoint *e) {
	// try to acquire next package synchronously
	auto error = receiveNow(e);
	switch (error) {
		case OK:
		case SYSTEM_ERROR:
		case INVALID_BUFFER:
			return error;

		case WOULD_BLOCK:
			startAsyncReceive(e);
		case TIMEOUT:
			_error = _recvFuture.get();
			return _error ? SYSTEM_ERROR : OK;

		default:
			return UNKNOWN_ERROR;
	}

}

Socket::ErrorCode Socket::receiveNow(Endpoint *e) {
	if (_readBuffer.data() == nullptr)
		return INVALID_BUFFER;

	if (_recvFuture.valid())
		return TIMEOUT;

	auto buf = asio::buffer((void *) _readBuffer.wBegin(), _readBuffer.wSize());
	size_t bytesRead;
	if (e)
		bytesRead = _socket.receive_from(buf, *e, 0, _error);
	else
		bytesRead = _socket.receive(buf, 0, _error);

	if (!_error) {
		_readBuffer.wSeekRel(bytesRead);
		return OK;
	} else if (_error == asio::error::would_block) {
		return WOULD_BLOCK;
	} else {
		return SYSTEM_ERROR;
	}

}

Socket::ErrorCode Socket::receiveFor(uint64 dur, Endpoint *e) {
	// try to acquire next package synchronously
	auto error = receiveNow(e);
	switch (error) {
		case OK:
		case SYSTEM_ERROR:
		case INVALID_BUFFER:
			return error;

		case WOULD_BLOCK:
			startAsyncReceive(e);
		case TIMEOUT:
			break;

		default:
			return UNKNOWN_ERROR;
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

Socket::ErrorCode Socket::receiveUntil(uint64 time, Endpoint *e) {
    Time now = getCurrentTime();
	return receiveFor(time - now, e);
}

void Socket::startAsyncReceive(Endpoint *e) {
	if (_recvFuture.valid()) {
		LOG(ERROR, "Tried to start new receive, but last one is still active");
	}

	_recvPromise = std::promise<boost::system::error_code>();
	_recvFuture = _recvPromise.get_future();
	auto buf = asio::buffer((void *) _readBuffer.wBegin(), _readBuffer.wSize());
	auto lambda = [this](const boost::system::error_code &err, size_t size) {
		if (!err) {
			_readBuffer.wSeekRel(size);
		}
		_recvPromise.set_value(err);
	};
	if (e) {
		_socket.async_receive_from(buf, *e, lambda);
	} else {
		_socket.async_receive(buf, lambda);
	}
}

Socket::ErrorCode Socket::send(const Endpoint *e) {
	// try to send packet synchronously
	auto error = sendNow(e);
	switch (error) {
	case OK:
	case SYSTEM_ERROR:
	case INVALID_BUFFER:
		return error;

	case WOULD_BLOCK:
		startAsyncSend(e);
	case TIMEOUT:
		_error = _sendFuture.get();
		return _error ? SYSTEM_ERROR : OK;

	default:
		return UNKNOWN_ERROR;
	}

}

Socket::ErrorCode Socket::sendNow(const Endpoint *e) {
	if (_writeBuffer.data() == nullptr)
		return INVALID_BUFFER;

	if (_sendFuture.valid())
		return TIMEOUT;

	auto buf = asio::buffer((const void *) _writeBuffer.rBegin(), _writeBuffer.rSize());
	size_t bytesSent;
	if (e)
		bytesSent = _socket.send_to(buf, *e, 0, _error);
	else
		bytesSent = _socket.send(buf, 0, _error);

	if (!_error) {
		_writeBuffer.rSeekRel(bytesSent);
		return OK;
	} else if (_error == asio::error::would_block) {
		return WOULD_BLOCK;
	} else {
		return SYSTEM_ERROR;
	}

}

Socket::ErrorCode Socket::sendFor(uint64 duration, const Endpoint *e) {
	// try to send packet synchronously
	auto error = sendNow(e);
	switch (error) {
		case OK:
		case SYSTEM_ERROR:
		case INVALID_BUFFER:
			return OK;

		case WOULD_BLOCK:
			startAsyncSend(e);
		case TIMEOUT:
			break;

		default:
			return UNKNOWN_ERROR;
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

Socket::ErrorCode Socket::sendUntil(uint64 time, const Endpoint *e) {
    time_t now = getCurrentTime();
	return sendFor(time - now, e);
}

void Socket::startAsyncSend(const Endpoint *e) {
	if (_sendFuture.valid()) {
		LOG(ERROR, "Tried to start new send, but last one is still active");
	}

	_sendPromise = std::promise<boost::system::error_code>();
	_sendFuture = _sendPromise.get_future();
	auto buf = asio::buffer((const void *) _writeBuffer.rBegin(), _writeBuffer.rSize());
	auto lambda = [this](const boost::system::error_code &err, size_t size) {
		if (!err) {
			_writeBuffer.rSeekRel(size);
		}
		_sendPromise.set_value(err);
	};
	if (e)
		_socket.async_send_to(buf, *e, lambda);
	else
		_socket.async_send(buf, lambda);
}

void Socket::acquireReadBuffer(Buffer &buffer) {
	_readBuffer = std::move(buffer);
}

void Socket::releaseReadBuffer(Buffer &buffer) {
	buffer = std::move(_readBuffer);
}

void Socket::acquireWriteBuffer(Buffer &buffer) {
	_writeBuffer = std::move(buffer);
}

void Socket::releaseWriteBuffer(Buffer &buffer) {
	buffer = std::move(_writeBuffer);
}

Socket::ErrorCode Socket::receive(Buffer &buffer, Endpoint *e) {
	acquireReadBuffer(buffer);
	auto error = receive(e);
	releaseReadBuffer(buffer);
	return error;
}

Socket::ErrorCode Socket::receiveNow(Buffer &buffer, Endpoint *e) {
	acquireReadBuffer(buffer);
	auto error = receiveNow(e);
	releaseReadBuffer(buffer);
	return error;
}

Socket::ErrorCode Socket::send(Buffer &buffer, const Endpoint *e) {
	acquireWriteBuffer(buffer);
	auto error = send(e);
	releaseWriteBuffer(buffer);
	return error;
}

Socket::ErrorCode Socket::sendNow(Buffer &buffer, const Endpoint *e) {
	acquireWriteBuffer(buffer);
	auto error = sendNow(e);
	releaseWriteBuffer(buffer);
	return error;
}

std::string getBoostErrorString(boost::system::error_code e) {
	// basic errors
	if (e == asio::error::access_denied)
		return "Permission denied";
	if (e == asio::error::address_family_not_supported)
		return "Address family not supported by protocol";
	if (e == asio::error::address_in_use)
		return "Address already in use";
	if (e == asio::error::already_connected)
		return "Transport endpoint is already connected";
	if (e == asio::error::already_started)
		return "Operation already in progress";
	if (e == asio::error::broken_pipe)
		return "Broken pipe";
	if (e == asio::error::connection_aborted)
		return "A connection has been aborted";
	if (e == asio::error::connection_refused)
		return "Connection refused";
	if (e == asio::error::connection_reset)
		return "Connection reset by peer";
	if (e == asio::error::bad_descriptor)
		return "Bad file descriptor";
	if (e == asio::error::fault)
		return "Bad address";
	if (e == asio::error::host_unreachable)
		return "No route to host";
	if (e == asio::error::in_progress)
		return "Operation now in progress";
	if (e == asio::error::interrupted)
		return "Interrupted system call";
	if (e == asio::error::invalid_argument)
		return "Invalid argument";
	if (e == asio::error::message_size)
		return "Message too long";
	if (e == asio::error::name_too_long)
		return "The name was too long";
	if (e == asio::error::network_down)
		return "Network is down";
	if (e == asio::error::network_reset)
		return "Network dropped connection on reset";
	if (e == asio::error::network_unreachable)
		return "Network is unreachable";
	if (e == asio::error::no_descriptors)
		return "Too many open files";
	if (e == asio::error::no_buffer_space)
		return "No buffer space available";
	if (e == asio::error::no_memory)
		return "Cannot allocate memory";
	if (e == asio::error::no_permission)
		return "Operation not permitted";
	if (e == asio::error::no_protocol_option)
		return "Protocol not available";
	if (e == asio::error::not_connected)
		return "Transport endpoint is not connected";
	if (e == asio::error::not_socket)
		return "Socket operation on non-socket";
	if (e == asio::error::operation_aborted)
		return "Operation cancelled";
	if (e == asio::error::operation_not_supported)
		return "Operation not supported";
	if (e == asio::error::shut_down)
		return "Cannot send after transport endpoint shutdown";
	if (e == asio::error::timed_out)
		return "Connection timed out";
	if (e == asio::error::try_again)
		return "Resource temporarily unavailable";
	if (e == asio::error::would_block)
		return "The socket is marked non-blocking "
		       "and the requested operation would block";

	return "Unknown error";
}
