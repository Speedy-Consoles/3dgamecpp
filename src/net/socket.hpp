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
		INVALID_BUFFER,
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

	ErrorCode receive(endpoint_t * = nullptr);
	ErrorCode receiveNow(endpoint_t * = nullptr);
	ErrorCode receiveFor(uint64 duration, endpoint_t * = nullptr);
	ErrorCode receiveUntil(uint64 time, endpoint_t * = nullptr);

	ErrorCode send(const endpoint_t * = nullptr);
	ErrorCode sendNow(const endpoint_t * = nullptr);
	ErrorCode sendFor(uint64 duration, const endpoint_t * = nullptr);
	ErrorCode sendUntil(uint64 time, const endpoint_t * = nullptr);

	void acquireReadBuffer(Buffer &);
	void releaseReadBuffer(Buffer &);

	void acquireWriteBuffer(Buffer &);
	void releaseWriteBuffer(Buffer &);

private:
	socket_t _socket;
	error_t _error;

	std::future<error_t> _recvFuture;
	std::promise<error_t> _recvPromise;

	std::future<error_t> _sendFuture;
	std::promise<error_t> _sendPromise;

	Buffer _readBuffer;
	Buffer _writeBuffer;

	void startAsyncReceive(endpoint_t *);
	void startAsyncSend(const endpoint_t *);
};

std::string getBoostErrorString(error_t);

}} // namespace my::net

#endif // SOCKET_HPP_
