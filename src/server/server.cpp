#include "server.hpp"

#include <thread>
#include <future>
#include <string>

#include <csignal>

#include <boost/filesystem.hpp>

#include "shared/engine/std_types.hpp"
#include "shared/engine/random.hpp"
#include "shared/block_utils.hpp"
#include "shared/net.hpp"

namespace {
	volatile std::sig_atomic_t closeRequested;
}

static void signalCallback(int) {
	closeRequested = 1;
}

static logging::Logger logger("server");

int main() {
	signal(SIGINT, &signalCallback);
	signal(SIGTERM, &signalCallback);

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

Server::Server(uint16 port, const char *worldId) {
	LOG_INFO(logger) << "Creating Server";

	save = std::unique_ptr<Save>(new Save(worldId));
	boost::filesystem::path path(save->getPath());
	if (!boost::filesystem::exists(path)) {
		boost::filesystem::create_directories(path);
		std::random_device rng;
		boost::random::uniform_int_distribution<uint64> distr;
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
	if (!host)
		return;

	LOG_INFO(logger) << "Running server";

	time = getCurrentTime();

	int tick = 0;
	while (!closeRequested) {
		updateNet();
		kickUnfriendly();

		world->tick(-1);
		chunkManager->tick();

		//TODO use makeSnapshot
		sendSnapshots(tick);

		// Time remTime = time + seconds(1) / TICK_SPEED - getCurrentTime();

		sync(TICK_SPEED);
		tick++;
	}

	LOG_INFO(logger) << "Shutting down server";

	host->duplicatePeers = 0;

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (clientInfos[i].status != INVALID && clientInfos[i].status != DISCONNECTING) {
			enet_peer_disconnect(clientInfos[i].peer, (enet_uint32) SERVER_SHUTDOWN);
			clientInfos[i].status = DISCONNECTING;
		}
	}

	Time endTime = getCurrentTime() + seconds(1);
	while (numClients > 0 && getCurrentTime() < endTime) {
		updateNetShutdown();
	}
}

void Server::updateNet() {
	ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			handleConnect(event.peer);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			handleDisconnect(event.peer, (DisconnectReason) event.data);
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

void Server::updateNetShutdown() {
	ENetEvent event;
	if (enet_host_service(host, &event, 10) > 0) {
		switch (event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			LOG_WARNING(logger) << "Received ENetEvent ENET_EVENT_TYPE_CONNECT while shutting down";
			enet_peer_reset(event.peer);
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			numClients--;
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			LOG_WARNING(logger) << "Ignoring ENetEvent ENET_EVENT_TYPE_RECEIVE while shutting down";
			enet_packet_destroy(event.packet);
			break;
		default:
			LOG_WARNING(logger) << "Received unknown ENetEvent type << event.type";
			break;
		}
	}
}

void Server::sendSnapshots(int tick) {
	Snapshot snapshot;
	snapshot.tick = tick;
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		PlayerSnapshot &playerSnapshot = *(snapshot.playerSnapshots + i);
		if (clientInfos[i].status != PLAYING) {
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
		if (clientInfos[i].status != PLAYING)
			continue;
		snapshot.localId = i;
		send(snapshot, i);
	}
}

void Server::kickUnfriendly() {
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (clientInfos[i].status == CONNECTING && getCurrentTime() > clientInfos[i].connectionStartTime + seconds(2)) {
			enet_peer_disconnect(clientInfos[i].peer, (enet_uint32) KICKED);
			clientInfos[i].status = DISCONNECTING;
		}
	}
}

void Server::handleConnect(ENetPeer *peer) {
	int id = -1;
	for (int i = 0; i < (int) MAX_CLIENTS; i++) {
		if (clientInfos[i].status == INVALID) {
			id = i;
			peer->data = new PeerData{id};
			clientInfos[id].peer = peer;
			clientInfos[id].status = CONNECTING;
			clientInfos[id].connectionStartTime = getCurrentTime();
			break;
		}
	}
	if (id == -1) {
		enet_peer_reset(peer);
		LOG_ERROR(logger) << "Could not find unused id for new player";
		return;
	}

	numClients++;

	LOG_DEBUG(logger) << "Incoming connection: " << id;
}

void Server::handleDisconnect(ENetPeer *peer, DisconnectReason reason) {
	int id = ((PeerData *) peer->data)->id;

	LOG_DEBUG(logger) << "Disconnected: " << (int) id;

	if(clientInfos[id].status == PLAYING)
		onPlayerLeave(id, reason);

	delete (PeerData *)peer->data;
	peer->data = nullptr;
	clientInfos[id].peer = nullptr;
	clientInfos[id].status = INVALID;

	numClients--;
}

void Server::handlePacket(const enet_uint8 *data, size_t size, size_t channel, ENetPeer *peer) {
	LOG_TRACE(logger) << "Received message of length " << size;

	MessageType type;
	if (readMessageHeader((const char *) data, size, &type)) {
		LOG_WARNING(logger) << "Received malformed message";
		return;
	}

	int id = ((PeerData *) peer->data)->id;
	LOG_TRACE(logger) << "Message type: " << (int) type << ", Player id: " << (int) id;
	switch (clientInfos[id].status) {
	case CONNECTING:
		if (type == PLAYER_INFO) {
			PlayerInfo info;
			if (readMessageBody((const char *) data, size, &info)) {
				LOG_WARNING(logger) << "Received malformed message";
				break;
			}
			clientInfos[id].name = info.name;
			clientInfos[id].status = PLAYING;
			onPlayerJoin(id);
		} else
			LOG_WARNING(logger) << "Received message of type " << type << " from player without info";
		break;
	case DISCONNECTING:
		LOG_WARNING(logger) << "Received message from disconnecting player";
		break;
	case PLAYING:
		switch (type) {
		case PLAYER_INPUT:
			{
				PlayerInput input;
				if (readMessageBody((const char *) data, size, &input)) {
					LOG_WARNING(logger) << "Received malformed message";
					break;
				}
				onPlayerInput(id, input);
			}
			break;
		default:
			LOG_WARNING(logger) << "Received message of unknown type " << type;
			break;
		}
		break;
	default:
		break;
	}
}

void Server::onPlayerJoin(int id) {
	world->addPlayer(id);

	PlayerJoinEvent pje;
	pje.id = id;
	broadcast(pje);

	LOG_INFO(logger) << clientInfos[id].name << " joined the game";
}

void Server::onPlayerLeave(int id, DisconnectReason reason) {
	world->deletePlayer(id);

	PlayerLeaveEvent ple;
	ple.id = id;
	broadcast(ple);

	switch (reason) {
	case TIMEOUT:
		LOG_INFO(logger) <<  clientInfos[id].name << " timed out";
		break;
	case CLIENT_LEAVE:
		LOG_INFO(logger) <<  clientInfos[id].name << " disconnected";
		break;
	default:
		LOG_WARNING(logger) << "Unexpected DisconnectReason " << reason;
		break;
	}
}

void Server::onPlayerInput(int id, PlayerInput &input) {
	int yaw = input.yaw;
	int pitch = input.pitch;
	world->getPlayer(id).setOrientation(yaw, pitch);
	world->getPlayer(id).setMoveInput(input.moveInput);
	world->getPlayer(id).setFly(input.flying);
}

void Server::sync(int perSecond) {
	time = time + seconds(1) / perSecond;
	sleepUntil(time);
}
