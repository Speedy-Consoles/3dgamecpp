#ifndef SERVER_HPP
#define SERVER_HPP

#include <memory>
#include <string>
#include <enet/enet.h>

#include "shared/net.hpp"
#include "shared/engine/logging.hpp"
#include "shared/engine/time.hpp"
#include "shared/constants.hpp"

class GameServer;

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
};

class Server {
private:
	std::unique_ptr<GameServer> gameServer;

	ClientInfo clientInfos[MAX_CLIENTS];

	int numClients = 0;

	ENetHost *host;

	// time keeping
	Time time = 0;

	int tick = 0;

public:
	Server(uint16 port, const char *worldId = "default");
	~Server();

	void run();

	int getTick() { return tick; }

	// TODO think about channels
	// maybe chat and events in different channels
	template<typename T> void send(T &msg, int clientId, bool reliable) {
		static logging::Logger logger("server");
		size_t size = getMessageSize(msg);
		enet_uint32 flags = 0;
		if (reliable)
			flags |= ENET_PACKET_FLAG_RELIABLE;
		ENetPacket *packet = enet_packet_create(nullptr, size, flags);
		if (writeMessage(msg, (char *) packet->data, size))
			LOG_ERROR(logger) << "Could not serialize message";

		enet_peer_send(clientInfos[clientId].peer, 0, packet);
	}

	template<typename T> void broadcast(T &msg, bool reliable) {
		static logging::Logger logger("server");
		size_t size = getMessageSize(msg);
		enet_uint32 flags = 0;
		if (reliable)
			flags |= ENET_PACKET_FLAG_RELIABLE;
		ENetPacket *packet = enet_packet_create(nullptr, size, flags);
		if (writeMessage(msg, (char *) packet->data, size))
			LOG_ERROR(logger) << "Could not serialize message";
		enet_host_broadcast(host, 0, packet);
	}

private:
	void updateNet();
	void updateNetShutdown();
	void kickUnfriendly();

	void handleConnect(ENetPeer *peer);
	void handleDisconnect(ENetPeer *peer, DisconnectReason reason);
	void handlePacket(const enet_uint8 *data, size_t size, size_t channel, ENetPeer *peer);

	void sync(int perSecond);
};

#endif // SERVER_HPP
