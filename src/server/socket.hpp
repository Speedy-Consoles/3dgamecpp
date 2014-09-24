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

	ErrorCode getErrorCode() const;
	const error_t &getSystemError() const;

	void open();
	void close();

	bool isOpen() const;

	void connect(const endpoint_t &endpoint);
	void bind(const endpoint_t &endpoint);

	void receive(Packet *);
	void receiveNow(Packet *);
	void receiveFor(Packet *, uint64 duration);
	void receiveUntil(Packet *, uint64 time);

	void send(const Packet &);
	void sendNow(const Packet &);
	void sendFor(const Packet &, uint64 duration);
	void sendUntil(const Packet &, uint64 time);

private:
	socket_t _socket;

	ErrorCode _ec = OK;
	error_t _error;

	std::future<void> _future;
	std::promise<void> _promise;

	void startAsyncReceive(Packet *p);
};

}} // namespace my::net

#endif // SOCKET_HPP_
