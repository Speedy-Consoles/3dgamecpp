#include "remote_server_interface.hpp"

#include <thread>

#include "shared/engine/logging.hpp"
#include "shared/saves.hpp"

#include "client.hpp"

using namespace std;
using namespace boost;
using namespace boost::asio::ip;

static logging::Logger logger("remote");

RemoteServerInterface::RemoteServerInterface(Client *client, std::string addressString) :
		client(client),
		worldGenerator(new WorldGenerator(42, WorldParams())),
		asyncWorldGenerator(worldGenerator.get()),
		inBuffer(1024*64),
		outBuffer(1024*64)
{
	if (enet_initialize() != 0) {
		LOG_FATAL(logger) << "An error occurred while initializing ENet.";
		status = CONNECTION_ERROR;
	}

	host = enet_host_create(NULL, 1, 1, 20000, 5000);
	if (!host) {
		LOG_ERROR(logger) << "An error occurred while trying to create an ENet client host.";
		status = CONNECTION_ERROR;
		return;
	}

	ENetAddress address;
	enet_address_set_host(&address, addressString.c_str());
	address.port = 8547;
	peer = enet_host_connect(host, &address, 1, 0);
	if (!peer) {
		LOG_ERROR(logger) << "No available peers for initiating an ENet connection.";
		status = CONNECTION_ERROR;
	}

	status = CONNECTING;
}

RemoteServerInterface::~RemoteServerInterface() {
	if(host)
		enet_host_destroy(host);
	enet_deinitialize();
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
	if (status == CONNECTING) {
		// check for connection success
		ENetEvent event;
		if (enet_host_service (host, &event, 0) > 0) {
			switch(event.type) {
			case ENET_EVENT_TYPE_CONNECT:
				LOG_INFO(logger) << "Successfully connected to server";
				status = CONNECTED;
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				enet_peer_reset (peer);
				peer = nullptr;
				LOG_ERROR(logger) << "Could not connect to server";
				status = CONNECTION_ERROR;
				break;
			default:
				LOG_WARNING(logger) << "Unexpected ENetEvent while connecting";
				break;
			}
		}
	}

	if (status != CONNECTED)
		return;

	// receive
	ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0) {
		switch(event.type) {
		case ENET_EVENT_TYPE_DISCONNECT:
			peer = nullptr;
			LOG_INFO(logger) << "Disconnected from server";
			status = NOT_CONNECTED;
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			handlePacket(event.packet->data, event.packet->dataLength, event.channelID);
			enet_packet_destroy(event.packet);
			break;
		case ENET_EVENT_TYPE_CONNECT:
			LOG_WARNING(logger) << "Unexpected ENetEvent ENET_EVENT_TYPE_CONNECT";
			break;
		default:
			LOG_WARNING(logger) << "Received unknown ENetEvent type";
			break;
		}
	}

	if (status != CONNECTED)
		return;

	// send
	ClientMessage cmsg;
	cmsg.type = PLAYER_INPUT;
	cmsg.playerInput.input.yaw = yaw;
	cmsg.playerInput.input.pitch = pitch;
	cmsg.playerInput.input.moveInput = moveInput;
	cmsg.playerInput.input.flying = flying;
	send(cmsg);
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

void RemoteServerInterface::handlePacket(const enet_uint8 *data, size_t size, size_t channel) {
	ServerMessage smsg;
	receive(&smsg, data, size);
	switch (smsg.type) {
	case PLAYER_JOIN_EVENT:
		// TODO
		break;
	case PLAYER_LEAVE_EVENT:
		// TODO
		break;
	case SNAPSHOT:
		localPlayerId = smsg.snapshot.localId;
		for (int i = 0; i < MAX_CLIENTS; ++i) {
			PlayerSnapshot &playerSnapshot = *(smsg.snapshot.playerSnapshots + i);
			Player &player = client->getWorld()->getPlayer(i);
			if (playerSnapshot.valid && !player.isValid())
				client->getWorld()->addPlayer(i);
			else if (!playerSnapshot.valid && player.isValid())
				client->getWorld()->deletePlayer(i);
			if (playerSnapshot.valid)
				player.applySnapshot(playerSnapshot, i == localPlayerId);
		}
		break;
	case MALFORMED_SERVER_MESSAGE:
		LOG_WARNING(logger) << "Received malformed message";
		break;
	default:
		LOG_WARNING(logger) << "Received message of unknown type " << smsg.type;
	}
}

void RemoteServerInterface::receive(ServerMessage *cmsg, const enet_uint8 *data, size_t size) {
	inBuffer.clear();
	inBuffer.write((const char *) data, size);
	inBuffer >> *cmsg;
}

void RemoteServerInterface::send(ClientMessage &cmsg) {
	// TODO make possible without buffer
	outBuffer.clear();
	outBuffer << cmsg;
	// TODO reliability, sequencing
	ENetPacket *packet = enet_packet_create((const void *) outBuffer.rBegin(), outBuffer.rSize(), 0);
	enet_peer_send(peer, 0, packet);
}
