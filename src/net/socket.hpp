/*
 * socket.hpp
 *
 *  Created on: 24.09.2014
 *      Author: lars
 */

#ifndef SOCKET_HPP_
#define SOCKET_HPP_

#include "buffer.hpp"

#include "time.hpp"
#include "std_types.hpp"

#include <boost/asio.hpp>
#include <future>

namespace my { namespace net {

using ios_t = boost::asio::io_service;
using socket_t = boost::asio::ip::udp::socket;
using endpoint_t = boost::asio::ip::udp::endpoint;
using error_t = boost::system::error_code;

class Socket {
public:
	enum ErrorCode {
		OK = 0,
		SYSTEM_ERROR,
		UNKNOWN_ERROR,
		WOULD_BLOCK,
		TIMEOUT,
		BUFFER_TOO_SMALL,
	};

	~Socket();
	Socket(ios_t &ios);
	Socket(ios_t &ios, const endpoint_t &endpoint);

	const error_t &getSystemError() const;

	ErrorCode open();
	void close();

	bool isOpen() const;

	ErrorCode connect(const endpoint_t &endpoint);
	ErrorCode bind(const endpoint_t &endpoint);

	ErrorCode receive(Buffer *, endpoint_t * = nullptr);
	ErrorCode receiveNow(Buffer *, endpoint_t * = nullptr);
	ErrorCode receiveFor(Buffer *, endpoint_t *, uint64 duration);
	ErrorCode receiveFor(Buffer *buf, uint64 duration) {return receiveFor(buf, nullptr, duration);}
	ErrorCode receiveUntil(Buffer *, endpoint_t *, uint64 time);
	ErrorCode receiveUntil(Buffer *buf, uint64 time) {return receiveUntil(buf, nullptr, time);}

	ErrorCode send(const Buffer &, const endpoint_t * = nullptr);
	ErrorCode sendNow(const Buffer &, const endpoint_t * = nullptr);
	ErrorCode sendFor(const Buffer &, const endpoint_t *, uint64 duration);
	ErrorCode sendFor(const Buffer &buf, uint64 duration) {return sendFor(buf, nullptr, duration);}
	ErrorCode sendUntil(const Buffer &, const endpoint_t *, uint64 time);
	ErrorCode sendUntil(const Buffer &buf, uint64 time) {return sendUntil(buf, nullptr, time);}

private:
	socket_t _socket;
	error_t _error;

	std::future<error_t> _recvFuture;
	std::promise<error_t> _recvPromise;

	std::future<error_t> _sendFuture;
	std::promise<error_t> _sendPromise;

	void startAsyncReceive(Buffer *, endpoint_t *);
	void startAsyncSend(const Buffer &, const endpoint_t *);
};

}} // namespace my::net

#endif // SOCKET_HPP_
