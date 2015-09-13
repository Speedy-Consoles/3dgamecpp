#include <thread>
#include <future>
#include <string>

#include <boost/asio.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/std_types.hpp"
#include "shared/engine/time.hpp"
#include "shared/engine/socket.hpp"
#include "shared/engine/buffer.hpp"
#include "shared/game/world.hpp"
#include "shared/block_utils.hpp"
#include "shared/constants.hpp"
#include "shared/net.hpp"

using namespace std;
using namespace boost;
using namespace boost::asio::ip;

static logging::Logger logger("server");

struct Client {
	bool connected;
	udp::endpoint endpoint;
	uint32 token;
	Time timeOfLastPacket;
};

class Server {
private:
	std::string id;
	bool closeRequested = false;
	World *world = nullptr;

	Client clients[MAX_CLIENTS];

    Time timeout = seconds(10);

	Buffer inBuf;
	Buffer outBuf;

	// time keeping
    Time time = 0;

	// our listening socket
	boost::asio::io_service ios;
	boost::asio::io_service::work *w;
	future<void> f;
	Socket socket;

public:
	Server(uint16 port, const char *worldId = "region");
	~Server();

	void run();

private:
	void handleMessage(const Endpoint &);
	void checkInactive();

	void handleConnectionRequest(const Endpoint &);
	void handleClientMessage(const ClientMessage &cmsg, uint8 id);

	void sendSnapshots(int tick);

	void disconnect(uint8 id);
};

int main() {
	logging::init("logging_srv.conf");

	LOG_TRACE(logger) << "Trace enabled";

	initUtil();
	Server server(8547);

	try {
		server.run();
	} catch (std::exception &e) {
		LOG_FATAL(logger) << "Exception: " << e.what();
		return -1;
	}

	return 0;

}

Server::Server(uint16 port, const char *worldId) :
	inBuf(1024*64),
	outBuf(1024*64),
	ios(),
	w(new boost::asio::io_service::work(ios)),
	socket(ios)
{
	LOG_INFO(logger) << "Creating Server";
	world = new World(worldId, nullptr); // TODO give chunk manager
	for (uint8 i = 0; i < MAX_CLIENTS; i++) {
		clients[i].connected = false;
		clients[i].timeOfLastPacket = 0;
	}

	socket.open();
	socket.bind(udp::endpoint(udp::v4(), port));

	// this will make sure our async_recv calls get handled
	f = async(launch::async, [this]{
		this->ios.run();
		LOG_INFO(logger) << "ASIO Thread returned";
	});
}

Server::~Server() {
	delete world;

	socket.close();
	delete w;
	if (f.valid())
		f.get();
}

void Server::run() {
	LOG_INFO(logger) << "Starting server";

	auto err = socket.getSystemError();
	if (err == asio::error::address_in_use) {
		LOG_FATAL(logger) << "Port already in use";
		return;
	} else if (err) {
		LOG_FATAL(logger) << "Unknown socket error!";
		return;
	}

	time = getCurrentTime();

	inBuf.clear();
	socket.acquireReadBuffer(inBuf);
	int tick = 0;
	while (!closeRequested) {
		world->tick(-1);

		time += seconds(1) / TICK_SPEED;
		Time remTime;
		while ((remTime = time - getCurrentTime() + seconds(1) / TICK_SPEED) > 0) {
			// TODO this can be overridden asynchronously
			Endpoint endpoint;
			switch (socket.receiveFor(remTime, &endpoint)) {
			case Socket::OK:
				socket.releaseReadBuffer(inBuf);
				handleMessage(endpoint);
				inBuf.clear();
				socket.acquireReadBuffer(inBuf);
				break;
			case Socket::TIMEOUT:
				break;
			case Socket::SYSTEM_ERROR:
				LOG_ERROR(logger) << "Could not receive on socket: "
						<< getBoostErrorString(socket.getSystemError());
				break;
			default:
				LOG_ERROR(logger) << "Unknown error while receiving packets";
			}
		}

		//TODO use makeSnapshot
		sendSnapshots(tick);

		checkInactive();
		tick++;
	}
	socket.releaseReadBuffer(inBuf);
	LOG_INFO(logger) << "Server is shutting down";
}

void Server::handleMessage(const Endpoint &endpoint) {
	size_t size = inBuf.rSize();
	LOG_TRACE(logger) << "Received message of length " << size;

	ClientMessage cmsg;
	inBuf >> cmsg;
	switch (cmsg.type) {
	case MALFORMED_CLIENT_MESSAGE:
		switch (cmsg.malformed.error) {
		case MESSAGE_TOO_SHORT:
			LOG_WARNING(logger) << "Message too short (" << size << ")";
			break;
		case WRONG_MAGIC:
			LOG_WARNING(logger) << "Incorrect magic";
			break;
		default:
			LOG_WARNING(logger) << "Unknown error reading message";
			break;
		}
		{
			char formatter[1024];
			char *head = formatter;
			for (const char *c = inBuf.data(); c < inBuf.rEnd(); ++c ) {
				sprintf(head, "%02x ", *c);
				head += 3;
			}
			head = '\0';
			LOG_DEBUG(logger) << formatter;
		}
		break;
	case CONNECTION_REQUEST:
		handleConnectionRequest(endpoint);
		break;
	default:
		int8 id = -1;
		for (int i = 0; i < (int) MAX_CLIENTS; ++i) {
			if (clients[i].connected && clients[i].endpoint == endpoint) {
				id = i;
				break;
			}
		}

		if (id >= 0) {
			clients[id].timeOfLastPacket = getCurrentTime();
			handleClientMessage(cmsg, id);
		} else {
			LOG_DEBUG(logger) << "Unknown remote address";

			ServerMessage smsg;
			smsg.type = CONNECTION_RESET;
			outBuf.clear();
			outBuf << smsg;
			switch (socket.send(outBuf, &endpoint)) {
			case Socket::OK:
				break;
			case Socket::SYSTEM_ERROR:
				LOG_ERROR(logger) << "Could not send on socket: "
						<< getBoostErrorString(socket.getSystemError());
				break;
			default:
				LOG_ERROR(logger) << "Unknown error while sending packet";
				break;
			}
		}
		break;
	}
}

void Server::handleConnectionRequest(const Endpoint &endpoint) {
	LOG_INFO(logger) << "New connection from " << endpoint.address().to_string() << ":" << endpoint.port();

	// find a free spot for the new player
	int newPlayer = -1;
	int duplicate = -1;
	for (int i = 0; i < (int) MAX_CLIENTS; i++) {
		bool connected = clients[i].connected;
		if (newPlayer == -1 && !connected) {
			newPlayer = i;
		} else if (connected && clients[i].endpoint == endpoint) {
			duplicate = i;
			break;
		}
	}

	// check for problems
	ServerMessage smsg;
	if (duplicate != -1) {
		LOG_INFO(logger) << "Duplicate remote endpoint with player " << duplicate;
		smsg.type = CONNECTION_REJECTED;
		smsg.conRejected.reason = DUPLICATE_ENDPOINT;
	} else if (newPlayer == -1) {
		LOG_INFO(logger) << "Server is full";
		smsg.type = CONNECTION_REJECTED;
		smsg.conRejected.reason = FULL;
	} else {
		uint8 id = (uint) newPlayer;

		world->addPlayer(id);

		clients[id].connected = true;
		clients[id].endpoint = endpoint;
		clients[id].timeOfLastPacket = getCurrentTime();

		smsg.type = CONNECTION_ACCEPTED;
		smsg.conAccepted.id = id;
		LOG_INFO(logger) << "New Player " << (int) id;
	}

	// send response
	outBuf.clear();
	outBuf << smsg;
	socket.send(outBuf, &endpoint);
}

void Server::handleClientMessage(const ClientMessage &cmsg, uint8 id) {
	LOG_TRACE(logger) << "Message " << (int) cmsg.type << " for Player " << (int) id << " accepted";
	switch (cmsg.type) {
	case ECHO_REQUEST:
	{
		ServerMessage smsg;
		smsg.type = ECHO_RESPONSE;
		outBuf.clear();
		outBuf << smsg << inBuf;
		socket.send(outBuf, &clients[id].endpoint);
		break;
	}
	case PLAYER_INPUT:
		{
			int yaw = cmsg.playerInput.input.yaw;
			int pitch = cmsg.playerInput.input.pitch;
			world->getPlayer(id).setOrientation(yaw, pitch);
			world->getPlayer(id).setMoveInput(cmsg.playerInput.input.moveInput);
			world->getPlayer(id).setFly(cmsg.playerInput.input.flying);
		}
		break;
	default:
		break;
	}
}

void Server::sendSnapshots(int tick) {
	for (uint8 id = 0; id < MAX_CLIENTS; ++id) {
		if (!clients[id].connected)
			continue;

		ServerMessage smsg;
		smsg.type = PLAYER_SNAPSHOT;
		smsg.playerSnapshot.id = id;
		smsg.playerSnapshot.snapshot.tick = tick;
		smsg.playerSnapshot.snapshot.pos = world->getPlayer(id).getPos();
		smsg.playerSnapshot.snapshot.vel = world->getPlayer(id).getVel();
		smsg.playerSnapshot.snapshot.yaw = (uint16) world->getPlayer(id).getYaw();
		smsg.playerSnapshot.snapshot.pitch = (int16) world->getPlayer(id).getPitch();
		smsg.playerSnapshot.snapshot.moveInput = world->getPlayer(id).getMoveInput();
		smsg.playerSnapshot.snapshot.isFlying = world->getPlayer(id).getFly();

		outBuf.clear();
		outBuf << smsg << inBuf;

		for (uint8 id2 = 0; id2 < MAX_CLIENTS; ++id2) {
			if (!clients[id2].connected)
				continue;
			socket.send(outBuf, &clients[id2].endpoint);
			outBuf.rSeek(0);
		}
	}
}

void Server::checkInactive() {
	for (uint8 id = 0; id < MAX_CLIENTS; ++id) {
		if (!clients[id].connected)
			continue;

		if (getCurrentTime() - clients[id].timeOfLastPacket > timeout) {
			LOG_INFO(logger) << "Player " << (int) id << " timed out";

			ServerMessage smsg;
			smsg.type = CONNECTION_TIMEOUT;
			outBuf.clear();
			outBuf << smsg;
			socket.send(outBuf, &clients[id].endpoint);

			disconnect(id);
		}
	}
}

void Server::disconnect(uint8 id) {
	world->deletePlayer(id);

	LOG_INFO(logger) << "Player " << (int) id << " disconnected";
	clients[id].connected = false;
}
