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

using Endpoint = boost::asio::ip::udp::endpoint;

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
	Socket(boost::asio::io_service &ios);
	Socket(boost::asio::io_service &ios, const Endpoint &endpoint);

	const boost::system::error_code &getSystemError() const;

	ErrorCode open();
	void close();

	bool isOpen() const;

	ErrorCode connect(const Endpoint &endpoint);
	ErrorCode bind(const Endpoint &endpoint);

	ErrorCode receive(Endpoint * = nullptr);
	ErrorCode receiveNow(Endpoint * = nullptr);
	ErrorCode receiveFor(uint64 duration, Endpoint * = nullptr);
	ErrorCode receiveUntil(uint64 time, Endpoint * = nullptr);

	ErrorCode send(const Endpoint * = nullptr);
	ErrorCode sendNow(const Endpoint * = nullptr);
	ErrorCode sendFor(uint64 duration, const Endpoint * = nullptr);
	ErrorCode sendUntil(uint64 time, const Endpoint * = nullptr);

	void acquireReadBuffer(Buffer &);
	void releaseReadBuffer(Buffer &);

	void acquireWriteBuffer(Buffer &);
	void releaseWriteBuffer(Buffer &);

	ErrorCode receive(Buffer &, Endpoint * = nullptr);
	ErrorCode receiveNow(Buffer &, Endpoint * = nullptr);
	ErrorCode send(Buffer &, const Endpoint * = nullptr);
	ErrorCode sendNow(Buffer &, const Endpoint * = nullptr);

private:
	boost::asio::ip::udp::socket _socket;
	boost::system::error_code _error;

	std::future<boost::system::error_code> _recvFuture;
	std::promise<boost::system::error_code> _recvPromise;

	std::future<boost::system::error_code> _sendFuture;
	std::promise<boost::system::error_code> _sendPromise;

	Buffer _readBuffer;
	Buffer _writeBuffer;

	void startAsyncReceive(Endpoint *);
	void startAsyncSend(const Endpoint *);
};

std::string getBoostErrorString(boost::system::error_code);

#endif // SOCKET_HPP_
