#include "remote_server_interface.hpp"

#include <thread>

#include "engine/logging.hpp"

#undef DEFAULT_LOGGER
#define DEFAULT_LOGGER NAMED_LOGGER("remote")

using namespace std;
using namespace boost;
using namespace boost::asio::ip;

RemoteServerInterface::RemoteServerInterface(World *world, const char *address, const GraphicsConf &conf) :
		conf(conf),
		world(world),
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
	f = async(launch::async, [this]{ this->ios.run(); });
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
		udp::resolver r(this->ios);
		udp::resolver::query q(udp::v4(), address, "");
		error_t err;
		udp::resolver::iterator iter = r.resolve(q, err);
		if (err || iter == udp::resolver::iterator()) {
			status = COULD_NOT_RESOLVE;
			return;
		}
		udp::endpoint ep = *iter;
		status = CONNECTING;
		LOG(INFO, "Connecting to " << ep.address().to_string());

		socket.connect(udp::endpoint(ep.address(), 8547));

		// TODO make connecting more error tolerant
		ClientMessage cmsg;
		cmsg.type = CONNECTION_REQUEST;
		outBuf.clear();
		outBuf << cmsg;

		socket.acquireWriteBuffer(outBuf);
		socket.send();
		socket.releaseWriteBuffer(outBuf);

		inBuf.clear();
		socket.acquireReadBuffer(inBuf);
		switch (socket.receiveFor(timeout)) {
		case Socket::OK:
		{
			socket.releaseReadBuffer(inBuf);
			ServerMessage smsg;
			inBuf >> smsg;
			switch (smsg.type) {
			case CONNECTION_ACCEPTED:
				localPlayerId = smsg.conAccepted.id;
				this->world->addPlayer(localPlayerId);
				this->world->getPlayer(localPlayerId).setFly(true);
				status = CONNECTED;
				LOG(INFO, "Connected to " << ep.address().to_string());
				break;
			case CONNECTION_REJECTED:
				if (smsg.conRejected.reason == FULL) {
					status = SERVER_FULL;
					break;
				} else {
					status = CONNECTION_ERROR;
					break;
				}
			case MALFORMED_SERVER_MESSAGE:
				switch (smsg.malformed.error) {
				case MESSAGE_TOO_SHORT:
					LOG(DEBUG, "Message too short");
					status = CONNECTION_ERROR;
					break;
				case WRONG_MAGIC:
					LOG(DEBUG, "Incorrect magic");
					status = CONNECTION_ERROR;
					break;
				default:
					LOG(DEBUG, "Unknown error reading message");
					status = CONNECTION_ERROR;
					break;
				}
				break;
			default:
				LOG(DEBUG, "Unexpected message");
				status = CONNECTION_ERROR;
				return;
			}
			break;
		}
		case Socket::TIMEOUT:
			status = TIMEOUT;
			LOG(INFO, "Connection to " << ep.address().to_string() << " timed out");
			break;
		default:
			status = CONNECTION_ERROR;
			LOG(ERROR, "Error while connecting");
			break;
		}
	});
}

ServerInterface::Status RemoteServerInterface::getStatus() {
	return status;
}

void RemoteServerInterface::toggleFly() {

}

void RemoteServerInterface::setPlayerMoveInput(int moveInput) {
	this->moveInput = moveInput;
}

void RemoteServerInterface::setPlayerOrientation(double yaw, double pitch) {

}

void RemoteServerInterface::setBlock(uint8 block) {

}

void RemoteServerInterface::edit(vec3i64 bc, uint8 type) {

}

void RemoteServerInterface::receive(uint64 timeLimit) {
	if (status != CONNECTED)
		return;
	Socket::ErrorCode error;
	inBuf.clear();
	while ((error = socket.receiveNow(inBuf)) == Socket::OK) {
		ServerMessage smsg;
		inBuf >> smsg;
		switch (smsg.type) {
		case ECHO_RESPONSE:
			printf("%s\n", inBuf.rBegin());
			break;
		case PLAYER_SNAPSHOT:
			// TODO also update other values
			printf("snapshot: %d\n", smsg.playerSnapshot.snapshot.moveInput);
			world->getPlayer(smsg.playerSnapshot.id).setMoveInput(smsg.playerSnapshot.snapshot.moveInput);
			break;
		default:
			printf("eh?\n");
			;//TODO
		}
		inBuf.clear();
	}
	if (error != Socket::WOULD_BLOCK) {
		LOG(ERROR, "Socket error number " << error << " while receiving");
	}
}

void RemoteServerInterface::sendInput() {
	if (status != CONNECTED)
		return;
	{
		ClientMessage cmsg;
		cmsg.type = ECHO_REQUEST;
		outBuf.clear();
		outBuf << cmsg << "wurst" << '\0';
		socket.send(outBuf);
	}
	{
		ClientMessage cmsg;
		cmsg.type = PLAYER_INPUT;
		cmsg.playerInput.input = moveInput;
		outBuf.clear();
		outBuf << cmsg;
		socket.send(outBuf);
	}
}

void RemoteServerInterface::setConf(const GraphicsConf &conf) {
	GraphicsConf old_conf = this->conf;
	this->conf = conf;

	if (conf.render_distance != old_conf.render_distance) {
		// TODO
	}
}

int RemoteServerInterface::getLocalClientId() {
	return localPlayerId;
}

void RemoteServerInterface::stop() {

}

