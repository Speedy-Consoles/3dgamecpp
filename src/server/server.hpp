#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>
#include <string>
#include <enet/enet.h>

#include "shared/engine/logging.hpp"
#include "shared/engine/time.hpp"
#include "shared/game/world.hpp"
#include "shared/saves.hpp"
#include "shared/constants.hpp"

#include "server_chunk_manager.hpp"

struct PeerData {
	int id;
};

enum ClientStatus {
	INVALID,
	CONNECTING,
	PLAYING,
	DISCONNECTING,
};

struct ClientInfo {
	ClientStatus status = INVALID;
	ENetPeer *peer = nullptr;
	Time connectionStartTime = 0;
	std::string name;
};

class Server {
private:
	std::unique_ptr<Save> save;
	std::unique_ptr<ServerChunkManager> chunkManager;
	std::unique_ptr<World> world;

	ClientInfo clientInfos[MAX_CLIENTS];

	int numClients = 0;

	ENetHost *host;

	// time keeping
	Time time = 0;

public:
	Server(uint16 port, const char *worldId = "default");
	~Server();

	void run();

private:
	void updateNet();
	void updateNetShutdown();
	void sendSnapshots(int tick);
	void kickUnfriendly();

	void handleConnect(ENetPeer *peer);
	void handleDisconnect(ENetPeer *peer, DisconnectReason reason);
	void handlePacket(const enet_uint8 *data, size_t size, size_t channel, ENetPeer *peer);

	void onPlayerJoin(int id);
	void onPlayerLeave(int id, DisconnectReason reason);
	void onPlayerInput(int id, PlayerInput &input);

	template<typename T> void send(T &msg, int clientId) {
		static logging::Logger logger("server");
		// TODO reliability, order
		size_t size = getMessageSize(msg);
		ENetPacket *packet = enet_packet_create(nullptr, size, 0);
		if (writeMessage(msg, (char *) packet->data, size))
			LOG_ERROR(logger) << "Could not serialize message";

		enet_peer_send(clientInfos[clientId].peer, 0, packet);
	}

	template<typename T> void broadcast(T &msg) {
		static logging::Logger logger("server");
		// TODO reliability, order
		size_t size = getMessageSize(msg);
		ENetPacket *packet = enet_packet_create(nullptr, size, 0);
		if (writeMessage(msg, (char *) packet->data, size))
			LOG_ERROR(logger) << "Could not serialize message";
		enet_host_broadcast(host, 0, packet);
	}

	void sync(int perSecond);
};

#endif // SERVER_HPP
