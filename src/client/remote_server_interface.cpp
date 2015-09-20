#include "remote_server_interface.hpp"

#include <thread>

#include "shared/engine/logging.hpp"
#include "shared/saves.hpp"

#include "client.hpp"

using namespace std;
using namespace boost;
using namespace boost::asio::ip;

static logging::Logger logger("remote");

RemoteServerInterface::RemoteServerInterface(Client *client, std::string address) :
		client(client),
		worldGenerator(new WorldGenerator(42, WorldParams())),
		asyncWorldGenerator(worldGenerator.get()),
		ios(),
		w(new boost::asio::io_service::work(ios)),
		socket(ios),
		status(NOT_CONNECTED),
		inBuf(1024*64),
		outBuf(1024*64)
{
	socket.open();
	socket.bind(udp::endpoint(udp::v4(), 0));

	auto err = socket.getSystemError();
	if (err == asio::error::address_in_use) {
		LOG_FATAL(logger) << "Port already in use";
		return;
	} else if(err) {
		LOG_FATAL(logger) << "Unknown socket error!";
		return;
	}

	// this will make sure our async_recv calls get handled
	f = async(launch::async, [this]{ this->ios.run(); });
	asyncConnect(address);
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
		LOG_INFO(logger) << "Resolving " << address;
		udp::resolver r(this->ios);
		udp::resolver::query q(udp::v4(), address, "");
		boost::system::error_code err;
		udp::resolver::iterator iter = r.resolve(q, err);
		if (err || iter == udp::resolver::iterator()) {
			status = COULD_NOT_RESOLVE;
			return;
		}
		udp::endpoint ep = *iter;
		status = CONNECTING;
		LOG_INFO(logger) << "Connecting to " << ep.address().to_string();

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
				client->getWorld()->addPlayer(localPlayerId);
				status = CONNECTED;
				LOG_INFO(logger) << "Connected to " << ep.address().to_string();
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
					LOG_DEBUG(logger) << "Message too short";
					status = CONNECTION_ERROR;
					break;
				case WRONG_MAGIC:
					LOG_DEBUG(logger) << "Incorrect magic";
					status = CONNECTION_ERROR;
					break;
				default:
					LOG_DEBUG(logger) << "Unknown error reading message";
					status = CONNECTION_ERROR;
					break;
				}
				break;
			default:
				LOG_DEBUG(logger) << "Unexpected message";
				status = CONNECTION_ERROR;
				return;
			}
			break;
		}
		case Socket::TIMEOUT:
			status = TIMEOUT;
			LOG_INFO(logger) << "Connection to " << ep.address().to_string() << " timed out";
			break;
		default:
			status = CONNECTION_ERROR;
			LOG_ERROR(logger) << "Error while connecting";
			break;
		}
	});
}

ServerInterface::Status RemoteServerInterface::getStatus() {
	return status;
}

void RemoteServerInterface::toggleFly() {
	flying = !flying;
}

void RemoteServerInterface::setPlayerMoveInput(int moveInput) {
	this->moveInput = moveInput;
}

void RemoteServerInterface::setPlayerOrientation(int yaw, int pitch) {
	this->yaw = yaw;
	this->pitch= pitch;
	if (status == CONNECTED)
		client->getWorld()->getPlayer(localPlayerId).setOrientation(yaw, pitch);
}

void RemoteServerInterface::setSelectedBlock(uint8 block) {
	if (block)
		return;
	else
		return;
}

void RemoteServerInterface::placeBlock(vec3i64 bc, uint8 type) {
	if (bc[0] == (int64) type)
		return;
	else
		return;
}

void RemoteServerInterface::tick() {
	// send
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
		cmsg.playerInput.input.yaw = yaw;
		cmsg.playerInput.input.pitch = pitch;
		cmsg.playerInput.input.moveInput = moveInput;
		cmsg.playerInput.input.flying = flying;
		outBuf.clear();
		outBuf << cmsg;
		socket.send(outBuf);
	}

	// receive
	Socket::ErrorCode error;
	inBuf.clear();
	while ((error = socket.receiveNow(inBuf)) == Socket::OK) {
		ServerMessage smsg;
		inBuf >> smsg;
		switch (smsg.type) {
		case ECHO_RESPONSE:
			//printf("%s\n", inBuf.rBegin());
			break;
		case CONNECTION_RESET:
			LOG_ERROR(logger) << "Server says we're not connected";
			// TODO disconnect
			break;
		case CONNECTION_TIMEOUT:
			LOG_ERROR(logger) << "Server says we timed out";
			// TODO disconnect
			break;
		case PLAYER_JOIN:
			if (smsg.playerJoin.id != localPlayerId)
				client->getWorld()->addPlayer(smsg.playerJoin.id);
			break;
		case PLAYER_LEAVE:
			if (smsg.playerLeave.id != localPlayerId)
				client->getWorld()->deletePlayer(smsg.playerLeave.id);
			break;
		case SNAPSHOT:
			client->getWorld()->getPlayer(smsg.playerSnapshot.id)
					.applySnapshot(smsg.playerSnapshot.snapshot, smsg.playerSnapshot.id == localPlayerId);
			break;
		case MALFORMED_SERVER_MESSAGE:
			LOG_WARNING(logger) << "Received malformed message";
			break;
		default:
			LOG_WARNING(logger) << "Received message of unknown type " << smsg.type;
		}
		inBuf.clear();
	}
	if (error != Socket::WOULD_BLOCK) {
		LOG_ERROR(logger) << "Socket error number " << error << " while receiving";
	}
}

void RemoteServerInterface::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.render_distance != old.render_distance) {
		// TODO
	}
}

int RemoteServerInterface::getLocalClientId() {
	return localPlayerId;
}

bool RemoteServerInterface::requestChunk(Chunk *chunk) {
	return asyncWorldGenerator.requestChunk(chunk);
}

Chunk *RemoteServerInterface::getNextChunk() {
	return asyncWorldGenerator.getNextChunk();
}
