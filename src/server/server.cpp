#include <thread>
#include <future>
#include <string>

#include <enet/enet.h>
#include <boost/filesystem.hpp>

#include "shared/engine/logging.hpp"
#include "shared/engine/std_types.hpp"
#include "shared/engine/time.hpp"
#include "shared/engine/socket.hpp"
#include "shared/engine/buffer.hpp"
#include "shared/game/world.hpp"
#include "shared/saves.hpp"
#include "shared/block_utils.hpp"
#include "shared/constants.hpp"
#include "shared/net.hpp"

#include "server_chunk_manager.hpp"

using namespace std;
using namespace boost;
using namespace boost::asio::ip;

static logging::Logger logger("server");

struct PeerData {
	int id;
};

struct Client {
	ENetPeer *peer = nullptr;
};

class Server {
private:
	bool closeRequested = false;

	std::unique_ptr<Save> save;
	std::unique_ptr<World> world;
	std::unique_ptr<ServerChunkManager> chunkManager;

	Client clients[MAX_CLIENTS];

	ENetHost *host;

	Buffer inBuffer;
	Buffer outBuffer;

	// time keeping
	Time time = 0;

public:
	Server(uint16 port, const char *worldId = "default");
	~Server();

	void run();

private:
	void updateNet();

	void handleConnect(ENetPeer *peer);
	void handleDisconnect(ENetPeer *peer);
	void handlePacket(const enet_uint8 *data, size_t size, size_t channel, ENetPeer *peer);

	void sendSnapshots(int tick);

	void receive(ClientMessage *cmsg, const enet_uint8 *data, size_t size);
	void send(ServerMessage &smsg, ENetPeer *peer);
	void send(ServerMessage &smsg, int clientId);
	void broadcast(ServerMessage &smsg);

	void sync(int perSecond);
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

// TODO correct buffer sizes
Server::Server(uint16 port, const char *worldId) : inBuffer(1024*64), outBuffer(1024*64) {
	LOG_INFO(logger) << "Creating Server";

	save = std::unique_ptr<Save>(new Save(worldId));
	boost::filesystem::path path(save->getPath());
	if (!boost::filesystem::exists(path)) {
		boost::filesystem::create_directories(path);
		std::random_device rng;
		uniform_int_distribution<uint64> distr;
		uint64 seed = distr(rng);
		save->initialize(worldId, seed);
		save->store();
	}

	ServerChunkManager *cm = new ServerChunkManager(save->getWorldGenerator(), save->getChunkArchive());
	chunkManager = std::unique_ptr<ServerChunkManager>(cm);
	world = std::unique_ptr<World>(new World(chunkManager.get()));

	if (enet_initialize() != 0)
		LOG_FATAL(logger) << "An error occurred while initializing ENet.";

	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = port;
	host = enet_host_create(&address, MAX_CLIENTS, 1, 0, 0);
	if (!host)
		LOG_FATAL(logger) << "An error occurred while trying to create an ENet server host.";
}

Server::~Server() {
	if (host)
		enet_host_destroy(host);
	enet_deinitialize();
}

void Server::run() {
	LOG_INFO(logger) << "Running server";

	time = getCurrentTime();

	int tick = 0;
	while (!closeRequested) {
		updateNet();

		world->tick(-1);
		chunkManager->tick();

		//TODO use makeSnapshot
		sendSnapshots(tick);

		// Time remTime = time + seconds(1) / TICK_SPEED - getCurrentTime();

		sync(TICK_SPEED);
		tick++;
	}

	LOG_INFO(logger) << "Shutting down server";
}

void Server::updateNet() {ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0) {
		Endpoint endpoint;
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			handleConnect(event.peer);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			handleDisconnect(event.peer);
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			handlePacket(
				event.packet->data,
				event.packet->dataLength,
				event.channelID,
				event.peer
				);
			enet_packet_destroy(event.packet);
			break;
		default:
			LOG_WARNING(logger) << "Received unknown ENetEvent type << event.type";
			break;
		}
	}
}

void Server::handleConnect(ENetPeer *peer) {
	int id = -1;
	for (int i = 0; i < (int) MAX_CLIENTS; i++) {
		if (!clients[i].peer) {
			id = i;
			peer->data = new PeerData{id};
			clients[id].peer = peer;
			break;
		}
	}
	if (id == -1) {
		enet_peer_reset(peer);
		LOG_FATAL(logger) << "Could not find unused id for new player";
		return;
	}

	world->addPlayer(id);

	ServerMessage smsg;
	smsg.type = PLAYER_JOIN_EVENT;
	// TODO remove this
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (i == id || !clients[i].peer)
			continue;
		smsg.playerJoinEvent.id = i;
		send(smsg, peer);
	}
	// TODO << this

	smsg.playerJoinEvent.id = id;
	broadcast(smsg);

	LOG_INFO(logger) << "New Player " << id;
}

void Server::handleDisconnect(ENetPeer *peer) {
	int id = ((PeerData *) peer->data)->id;
	delete (PeerData *)peer->data;
	peer->data = nullptr;
	clients[id].peer = nullptr;

	world->deletePlayer(id);

	ServerMessage smsg;
	smsg.type = PLAYER_LEAVE_EVENT;
	smsg.playerJoinEvent.id = id;
	broadcast(smsg);

	LOG_INFO(logger) << "Player " << (int) id << " disconnected";
}

void Server::handlePacket(const enet_uint8 *data, size_t size, size_t channel, ENetPeer *peer) {
	LOG_TRACE(logger) << "Received message of length " << size;

	// TODO any checks needed here? magic?
	ClientMessage cmsg;
	receive(&cmsg, data, size);
	int id = ((PeerData *) peer->data)->id;
	LOG_TRACE(logger) << "Message type: " << (int) cmsg.type << ", Player id: " << (int) id;
	switch (cmsg.type) {
	case PLAYER_INPUT:
		{
			int yaw = cmsg.playerInput.input.yaw;
			int pitch = cmsg.playerInput.input.pitch;
			world->getPlayer(id).setOrientation(yaw, pitch);
			world->getPlayer(id).setMoveInput(cmsg.playerInput.input.moveInput);
			world->getPlayer(id).setFly(cmsg.playerInput.input.flying);
		}
		break;
	case MALFORMED_CLIENT_MESSAGE:
		LOG_WARNING(logger) << "Received malformed message";
		break;
	default:
		LOG_WARNING(logger) << "Received message of unknown type " << cmsg.type;
		break;
	}
}

void Server::sendSnapshots(int tick) {
	ServerMessage smsg;
	smsg.type = SNAPSHOT;
	smsg.snapshot.tick = tick;
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		PlayerSnapshot &playerSnapshot = *(smsg.snapshot.playerSnapshots + i);
		if (!clients[i].peer) {
			playerSnapshot.valid = false;
			playerSnapshot.pos = vec3i64(0, 0, 0);
			playerSnapshot.vel = vec3d(0.0, 0.0, 0.0);
			playerSnapshot.yaw = 0;
			playerSnapshot.pitch = 0;
			playerSnapshot.moveInput = 0;
			playerSnapshot.isFlying = false;
			continue;
		}
		playerSnapshot.valid = true;
		playerSnapshot.pos = world->getPlayer(i).getPos();
		playerSnapshot.vel = world->getPlayer(i).getVel();
		playerSnapshot.yaw = (uint16) world->getPlayer(i).getYaw();
		playerSnapshot.pitch = (int16) world->getPlayer(i).getPitch();
		playerSnapshot.moveInput = world->getPlayer(i).getMoveInput();
		playerSnapshot.isFlying = world->getPlayer(i).getFly();
	}

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (!clients[i].peer)
			continue;
		smsg.snapshot.localId = i;
		send(smsg, i);
	}
}

void Server::receive(ClientMessage *cmsg, const enet_uint8 *data, size_t size) {
	inBuffer.clear();
	inBuffer.write((const char *) data, size);
	inBuffer >> *cmsg;
}

void Server::send(ServerMessage &smsg, ENetPeer *peer) {
	// TODO make possible without buffer
	outBuffer.clear();
	outBuffer << smsg;
	// TODO reliability, sequencing
	ENetPacket *packet = enet_packet_create((const void *) outBuffer.rBegin(), outBuffer.rSize(), 0);
	enet_peer_send(peer, 0, packet);
}

void Server::send(ServerMessage &smsg, int clientId) {
	send(smsg, clients[clientId].peer);
}

void Server::broadcast(ServerMessage &smsg) {
	// TODO make possible without buffer
	outBuffer.clear();
	outBuffer << smsg;
	// TODO reliability, sequencing
	ENetPacket *packet = enet_packet_create((const void *) outBuffer.rBegin(), outBuffer.rSize(), 0);
	enet_host_broadcast(host, 0, packet);
}

void Server::sync(int perSecond) {
	time = time + seconds(1) / perSecond;
	sleepUntil(time);
}
