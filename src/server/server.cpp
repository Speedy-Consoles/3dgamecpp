#include "game/world.hpp"
#include "io/logging.hpp"
#include "std_types.hpp"
#include "util.hpp"
#include "constants.hpp"
#include "time.hpp"
#include "net/net.hpp"
#include "net/socket.hpp"
#include "net/buffer.hpp"

#include <boost/asio.hpp>
#include <thread>
#include <future>
#include <string>

using namespace std;
using namespace boost;
using namespace boost::asio::ip;

using namespace my::time;
using namespace my::net;

#undef DEFAULT_LOGGER
#define DEFAULT_LOGGER NAMED_LOGGER("server")

struct Client {
	bool connected;
	udp::endpoint endpoint;
	uint32 token;
	my::time::time_t timeOfLastPacket;
};

class Server {
private:
	std::string id;
	bool closeRequested = false;
	World *world = nullptr;

	Client clients[MAX_CLIENTS];

	my::time::time_t timeout = my::time::seconds(10);

	Buffer inBuf;
	Buffer outBuf;

	// time keeping
	my::time::time_t time = 0;

	// our listening socket
	my::net::ios_t ios;
	ios_t::work *w;
	future<void> f;
	my::net::Socket socket;

public:
	Server(uint16 port, const char *worldId = "region");
	~Server();

	void run();

private:
	void handleMessage(const endpoint_t &);
	void checkInactive();

	void handleConnectionRequest(const endpoint_t &);
	void handleClientMessage(const ClientMessage &cmsg, uint8 id);

	void sendSnapshots(int tick);

	void disconnect(uint8 id);
};

int main(int argc, char *argv[]) {
	initLogging("logging_srv.conf");

	LOG(TRACE, "Trace enabled");

	initUtil();
	Server server(8547);

	try {
		server.run();
	} catch (std::exception &e) {
		LOG(FATAL, "Exception: " << e.what());
		return -1;
	}

	return 0;

}

Server::Server(uint16 port, const char *worldId) :
	inBuf(1024*64),
	outBuf(1024*64),
	ios(),
	w(new ios_t::work(ios)),
	socket(ios)
{
	LOG(INFO, "Creating Server");
	world = new World(worldId);
	for (uint8 i = 0; i < MAX_CLIENTS; i++) {
		clients[i].connected = false;
		clients[i].timeOfLastPacket = 0;
	}

	socket.open();
	socket.bind(udp::endpoint(udp::v4(), port));

	// this will make sure our async_recv calls get handled
	f = async(launch::async, [this]{
		this->ios.run();
		LOG(INFO, "ASIO Thread returned");
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
	LOG(INFO, "Starting server");

	auto err = socket.getSystemError();
	if (err == asio::error::address_in_use) {
		LOG(FATAL, "Port already in use");
		return;
	} else if (err) {
		LOG(FATAL, "Unknown socket error!");
		return;
	}

	time = my::time::now();

	inBuf.clear();
	socket.acquireReadBuffer(inBuf);
	int tick = 0;
	while (!closeRequested) {
		world->tick(tick, -1);

		time += seconds(1) / TICK_SPEED;
		my::time::time_t remTime;
		while ((remTime = time - now() + seconds(1) / TICK_SPEED) > 0) {
			// TODO this can be overridden asynchronously
			endpoint_t endpoint;
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
				LOG(ERROR, "Could not receive on socket: "
						<< getBoostErrorString(socket.getSystemError()));
				break;
			default:
				LOG(ERROR, "Unknown error while receiving packets");
			}
		}

		//TODO use makeSnapshot
		sendSnapshots(tick);

		checkInactive();
		tick++;
	}
	socket.releaseReadBuffer(inBuf);
	LOG(INFO, "Server is shutting down");
}

void Server::handleMessage(const endpoint_t &endpoint) {
	size_t size = inBuf.rSize();
	LOG(TRACE, "Received message of length " << size);

	ClientMessage cmsg;
	inBuf >> cmsg;
	switch (cmsg.type) {
	case MALFORMED_CLIENT_MESSAGE:
		switch (cmsg.malformed.error) {
		case MESSAGE_TOO_SHORT:
			LOG(WARNING, "Message too short (" << size << ")");
			break;
		case WRONG_MAGIC:
			LOG(WARNING, "Incorrect magic");
			break;
		default:
			LOG(WARNING, "Unknown error reading message");
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
			LOG(DEBUG, formatter);
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
			clients[id].timeOfLastPacket = my::time::now();
			handleClientMessage(cmsg, id);
		} else {
			LOG(DEBUG, "Unknown remote address");

			ServerMessage smsg;
			smsg.type = CONNECTION_RESET;
			outBuf.clear();
			outBuf << smsg;
			switch (socket.send(outBuf, &endpoint)) {
			case Socket::OK:
				break;
			case Socket::SYSTEM_ERROR:
				LOG(ERROR, "Could not send on socket: "
						<< getBoostErrorString(socket.getSystemError()));
				break;
			default:
				LOG(ERROR, "Unknown error while sending packet");
				break;
			}
		}
		break;
	}
}

void Server::handleConnectionRequest(const endpoint_t &endpoint) {
	LOG(INFO, "New connection from " << endpoint.address().to_string() << ":" << endpoint.port());

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
		LOG(INFO, "Duplicate remote endpoint with player " << duplicate);
		smsg.type = CONNECTION_REJECTED;
		smsg.conRejected.reason = DUPLICATE_ENDPOINT;
	} else if (newPlayer == -1) {
		LOG(INFO, "Server is full");
		smsg.type = CONNECTION_REJECTED;
		smsg.conRejected.reason = FULL;
	} else {
		uint8 id = (uint) newPlayer;

		world->addPlayer(id);

		clients[id].connected = true;
		clients[id].endpoint = endpoint;
		clients[id].timeOfLastPacket = my::time::now();

		smsg.type = CONNECTION_ACCEPTED;
		smsg.conAccepted.id = id;
		LOG(INFO, "New Player " << (int) id);
	}

	// send response
	outBuf.clear();
	outBuf << smsg;
	socket.send(outBuf, &endpoint);
}

void Server::handleClientMessage(const ClientMessage &cmsg, uint8 id) {
	LOG(TRACE, "Message " << (int) cmsg.type << " for Player " << (int) id << " accepted");
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
		world->getPlayer(id).setMoveInput(cmsg.playerInput.input);
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
		smsg.playerSnapshot.snapshot.yaw = (uint16) round(world->getPlayer(id).getYaw() * 100);
		smsg.playerSnapshot.snapshot.pitch = (int16) round(world->getPlayer(id).getPitch() * 100);
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

		if (my::time::now() - clients[id].timeOfLastPacket > timeout) {
			LOG(INFO, "Player " << (int) id << " timed out");

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

	LOG(INFO, "Player " << (int) id << " disconnected");
	clients[id].connected = false;
}
