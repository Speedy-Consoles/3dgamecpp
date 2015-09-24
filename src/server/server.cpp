#include "server.hpp"

#include <string>
#include <csignal>
#include <boost/filesystem.hpp>

#include "shared/engine/std_types.hpp"
#include "shared/engine/random.hpp"
#include "shared/block_utils.hpp"
#include "shared/net.hpp"

#include "game_server.hpp"
#include "chunk_server.hpp"

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

	LOG_INFO(logger) << "Creating server";

	gameServer = std::unique_ptr<GameServer>(new GameServer(this));
	chunkServer = std::unique_ptr<ChunkServer>(new ChunkServer(this));

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

	while (!closeRequested) {
		updateNet();
		kickUnfriendly();

		gameServer->tick();
		chunkServer->tick();

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

void Server::finishChunkMessageJob(ChunkMessageJob job) {
	size_t size = getMessageSize(job.message);
	enet_packet_resize(job.packet, size);
	if (writeMessageMeta(job.message, (char *) job.packet->data, size))
		LOG_ERROR(logger) << "Could not serialize message";
	enet_peer_send(clientInfos[job.clientId].peer, 0, job.packet);
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
		LOG_ERROR(logger) << "Could not find unused id for new client";
		return;
	}

	numClients++;

	LOG_DEBUG(logger) << "Incoming connection: " << id;
}

void Server::handleDisconnect(ENetPeer *peer, DisconnectReason reason) {
	int id = ((PeerData *) peer->data)->id;

	LOG_DEBUG(logger) << "Disconnected: " << (int) id;

	if(clientInfos[id].status == PLAYING)
		gameServer->onPlayerLeave(id, reason);

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
	LOG_TRACE(logger) << "Message type: " << (int) type << ", client id: " << (int) id;
	switch (clientInfos[id].status) {
	case CONNECTING:
		if (type == PLAYER_INFO) {
			PlayerInfo info;
			if (readMessageBody((const char *) data, size, &info)) {
				LOG_WARNING(logger) << "Received malformed message";
				break;
			}
			clientInfos[id].status = PLAYING;
			gameServer->onPlayerJoin(id, info);
		} else
			LOG_WARNING(logger) << "Received message of type " << type << " from client without player info";
		break;
	case DISCONNECTING:
		LOG_WARNING(logger) << "Received message from disconnecting client";
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
				gameServer->onPlayerInput(id, input);
			}
			break;
		case CHUNK_REQUEST:
			{
				// TODO let another thread encode the chunk
				ChunkRequest request;
				if (readMessageBody((const char *) data, size, &request)) {
					LOG_WARNING(logger) << "Received malformed message";
					break;
				}
				ChunkMessageJob job;
				size_t packetSize = Chunk::SIZE * sizeof(uint8);
				job.packet = enet_packet_create(nullptr, packetSize, ENET_PACKET_FLAG_RELIABLE);
				job.message.encodedBlocks = getEncodedBlocksPointer((char *) job.packet->data);
				job.clientId = id;
				chunkServer->onChunkRequest(request, job);
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

void Server::sync(int perSecond) {
	time = time + seconds(1) / perSecond;
	sleepUntil(time);
}
