#include "remote_server_interface.hpp"

#include <thread>

#include "io/logging.hpp"

#undef DEFAULT_LOGGER
#define DEFAULT_LOGGER NAMED_LOGGER("remote")

using namespace std;
using namespace my::net;
using namespace boost;
using namespace boost::asio::ip;

RemoteServerInterface::RemoteServerInterface(const char *address, const GraphicsConf &conf) :
		conf(conf),
		ios(),
		w(new ios_t::work(ios)),
		socket(ios),
		status(NOT_CONNECTED),
		inBuf(1024*64),
		outBuf(1024*64)
{

	socket.open();
	socket.bind(udp::endpoint(udp::v4(), 0));

	auto err = socket.getSystemError();
	if (err == asio::error::address_in_use) {
		LOG(FATAL, "Port already in use");
		return;
	} else if(err) {
		LOG(FATAL, "Unknown socket error!");
		return;
	}

	// this will make sure our async_recv calls get handled
	f = async(launch::async, [this]{ ios.run(); });
	asyncConnect(std::string(address));
}


RemoteServerInterface::~RemoteServerInterface() {
	socket.close();
	delete w;
	if (f.valid())
		f.get();
}

void RemoteServerInterface::asyncConnect(std::string address) {
	connectFuture = std::async(std::launch::async, [this, address]() {
		status = RESOLVING;
		LOG(INFO, "Resolving " << address);
		udp::resolver r(ios);
		udp::resolver::query q(udp::v4(), address, "");
		my::net::error_t err;
		auto iter = r.resolve(q, err);
		if (err || iter == udp::resolver::iterator()) {
			status = COULD_NOT_RESOLVE;
			return;
		}
		udp::endpoint ep = *iter;
		status = CONNECTING;
		LOG(INFO, "Connecting to " << ep.address().to_string());

		socket.connect(udp::endpoint(ep.address(), 8547));

		outBuf.clear();
		outBuf << MAGIC << CONNECTION_REQUEST;
		socket.send(outBuf);

		// TODO make more error tolerant
		inBuf.clear();
		switch (socket.receiveFor(&inBuf, timeout)) {
		case Socket::OK:
		{
			MessageHeader header;
			if (inBuf.rSize() < sizeof (MessageHeader)) {
				LOG(DEBUG, "Message too short for basic protocol header");
				return;
			}
			inBuf >> header;
			if (!checkMagic(header)) {
				LOG(DEBUG, "Incorrect magic");
				return;
			}

			switch (header.type) {
			case CONNECTION_ACCEPTED:
				if (inBuf.rSize() < sizeof (ConnectionAcceptedResponse)) {
					LOG(DEBUG, "Message stopped abruptly");
					return;
				}
				ConnectionAcceptedResponse msg;
				inBuf >> msg;
				localPlayerId = msg.id;
				token = msg.token;
				status = CONNECTED;
				LOG(INFO, "Connected to " << ep.address().to_string());
				return;
			case CONNECTION_REJECTED:
			{
				if (inBuf.rSize() < sizeof (ConnectionRejectedResponse)) {
					LOG(DEBUG, "Message stopped abruptly");
					return;
				}
				ConnectionRejectedResponse msg;
				inBuf >> msg;
				if (msg.reason == SERVER_FULL) {
					status = SERVER_FULL;
					return;
				} else {
					status = UNKNOWN_ERROR;
					return;
				}
			}
			default:
				LOG(DEBUG, "Unexpected message");
				return;
			}
			break;
		}
		case Socket::TIMEOUT:
			status = TIMEOUT;
			LOG(INFO, "Connection to " << ep.address().to_string() << " timed out");
			break;
		default:
			status = UNKNOWN_ERROR;
			LOG(ERROR, "Error while connecting");
			break;
		}
	});
}

ServerInterface::Status RemoteServerInterface::getStatus() {
	return status;
}

void RemoteServerInterface::togglePlayerFly() {

}

void RemoteServerInterface::setPlayerMoveInput(int moveInput) {

}

void RemoteServerInterface::setPlayerOrientation(double yaw, double pitch) {

}

void RemoteServerInterface::setBlock(uint8 block) {

}

void RemoteServerInterface::edit(vec3i64 bc, uint8 type) {

}

void RemoteServerInterface::receiveChunks(uint64 timeLimit) {
	if (status != CONNECTED)
		return;
	inBuf.clear();
	while (socket.receiveNow(&inBuf) == Socket::OK) {
		inBuf.rSeek(5);
		printf("%s\n", inBuf.rBegin());
		inBuf.clear();
	}
}

void RemoteServerInterface::sendInput() {
	if (status != CONNECTED)
		return;
	outBuf.clear();
	outBuf << MAGIC << ECHO_REQUEST << token << "wurst" << '\0';
	socket.send(outBuf);
}

void RemoteServerInterface::setConf(const GraphicsConf &conf) {
	GraphicsConf old_conf = this->conf;
	this->conf = conf;

	if (conf.render_distance != old_conf.render_distance) {
		// TODO
	}
}

int RemoteServerInterface::getLocalClientID() {
	return localPlayerId;
}

void RemoteServerInterface::stop() {

}

