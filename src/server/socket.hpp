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

struct Packet {
	Buffer *buf;
	endpoint_t endpoint;
};

class Socket {
public:
	enum ErrorCode {
		OK = 0,
		UNKNOWN_ERROR,
		WOULD_BLOCK,
		TIMEOUT,
		BUFFER_TOO_SMALL,
	};

	~Socket();
	Socket(ios_t &ios);
	Socket(ios_t &ios, const endpoint_t &endpoint);

	const error_t &getSystemError() const;

	void open();
	void close();

	bool isOpen() const;

	void connect(const endpoint_t &endpoint);
	void bind(const endpoint_t &endpoint);

	ErrorCode receive(Packet *);
	ErrorCode receiveNow(Packet *);
	ErrorCode receiveFor(Packet *, uint64 duration);
	ErrorCode receiveUntil(Packet *, uint64 time);

	ErrorCode send(const Packet &);
	ErrorCode sendNow(const Packet &);
	ErrorCode sendFor(const Packet &, uint64 duration);
	ErrorCode sendUntil(const Packet &, uint64 time);

private:
	socket_t _socket;
	error_t _error;

	std::future<error_t> _recvFuture;
	std::promise<error_t> _recvPromise;

	std::future<error_t> _sendFuture;
	std::promise<error_t> _sendPromise;

	void startAsyncReceive(Packet *p);
	void startAsyncSend(const Packet &);
};

}} // namespace my::net

#endif // SOCKET_HPP_
