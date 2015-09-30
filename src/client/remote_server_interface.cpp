#include "remote_server_interface.hpp"

#include <thread>

#include "shared/engine/time.hpp"
#include "shared/engine/logging.hpp"
#include "shared/chunk_compression.hpp"
#include "shared/saves.hpp"

#include "client.hpp"

using namespace std;

static logging::Logger logger("remote");

RemoteServerInterface::RemoteServerInterface(Client *client, std::string addressString) :
		client(client),
		requestedChunks(0, vec3i64HashFunc),
		worldGenerator(new WorldGenerator(42, WorldParams())),
		asyncWorldGenerator(worldGenerator.get())
{
	if (enet_initialize() != 0) {
		LOG_FATAL(logger) << "An error occurred while initializing ENet.";
		status = CONNECTION_ERROR;
		return;
	}

	host = enet_host_create(NULL, 1, NUM_CHANNELS, 0, 0);
	if (!host) {
		LOG_ERROR(logger) << "An error occurred while trying to create an ENet client host.";
		status = CONNECTION_ERROR;
		return;
	}

	LOG_INFO(logger) << "Connecting to " << addressString;
	ENetAddress address;
	enet_address_set_host(&address, addressString.c_str());
	address.port = 8547;
	peer = enet_host_connect(host, &address, NUM_CHANNELS, 0);
	if (!peer) {
		LOG_ERROR(logger) << "No available peers for initiating an ENet connection.";
		status = CONNECTION_ERROR;
		return;
	}

	status = CONNECTING;
}

RemoteServerInterface::~RemoteServerInterface() {
	if(host) {
		if (status == CONNECTING) {
			enet_peer_reset(peer);
			status = NOT_CONNECTED;
		}
		if (status == CONNECTED) {
			enet_peer_disconnect(peer, (enet_uint32) CLIENT_LEAVE);
			status = DISCONNECTING;
		}
		Time endTime = getCurrentTime() + seconds(1);
		while (status == DISCONNECTING && getCurrentTime() < endTime) {
			tick();
			sleepFor(millis(10));
		}
		if (status == DISCONNECTING) {
			enet_peer_reset(peer);
			LOG_INFO(logger) << "Forcefully disconnected from server";
			peer = nullptr;
			status = NOT_CONNECTED;
		}

		enet_host_destroy(host);
	}
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

void RemoteServerInterface::setCharacterOrientation(int yaw, int pitch) {
	this->yaw = yaw;
	this->pitch= pitch;
	if (status == CONNECTED)
		client->getWorld()->getCharacter(localCharacterId).setOrientation(yaw, pitch);
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
	if (status == CONNECTING)
		updateNetConnecting();
	else if(status == DISCONNECTING)
		updateNetDisconnecting();

	if (status != CONNECTED && status != WAITING_FOR_SNAPSHOT)
		return;

	// receive
	updateNet();

	if (status != CONNECTED)
		return;

	// send input
	PlayerInput input;
	input.yaw = yaw;
	input.pitch = pitch;
	input.moveInput = moveInput;
	input.flying = flying;
	send(input, CHANNEL_STATE, false);
}

void RemoteServerInterface::setConf(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.render_distance != old.render_distance) {
		// TODO
	}
}

int RemoteServerInterface::getLocalClientId() {
	return localCharacterId;
}

bool RemoteServerInterface::requestChunk(Chunk *chunk, bool cached, uint32 cachedRevision) {
	ChunkRequest msg;
	msg.coords = chunk->getCC();
	msg.cached = cached;
	msg.cachedRevision = cachedRevision;
	send(msg, CHANNEL_BLOCK_DATA, true);
	requestedChunks.insert({chunk->getCC(), chunk});
	return true;
}

Chunk *RemoteServerInterface::getNextChunk() {
	if (receivedChunks.empty())
		return nullptr;

	Chunk *chunk = receivedChunks.front();
	receivedChunks.pop();
	return chunk;
}

void RemoteServerInterface::updateNet() {
	ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0) {
		switch(event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			LOG_WARNING(logger) << "Unexpected ENetEvent ENET_EVENT_TYPE_CONNECT";
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			peer = nullptr;
			switch ((DisconnectReason) event.data) {
			case TIMEOUT:
				LOG_INFO(logger) << "Connection time out";
				break;
			case KICKED:
				LOG_INFO(logger) << "Kicked from server";
				break;
			case SERVER_SHUTDOWN:
				LOG_INFO(logger) << "Server shutting down";
				break;
			default:
				LOG_WARNING(logger) << "Unexpected DisconnectReason " << (DisconnectReason) event.data;
				break;
			}
			status = NOT_CONNECTED;
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			handlePacket(event.packet->data, event.packet->dataLength, event.channelID);
			enet_packet_destroy(event.packet);
			break;
		default:
			LOG_WARNING(logger) << "Received unknown ENetEvent type";
			break;
		}
	}
}

void RemoteServerInterface::updateNetConnecting() {
	ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0) {
		switch(event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			LOG_INFO(logger) << "Connected to server";
			status = WAITING_FOR_SNAPSHOT;
			{
				PlayerInfo info;
				info.name = "Unnamed player";
				send(info, CHANNEL_CHAT, true);
			}
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			enet_peer_reset(peer);
			peer = nullptr;
			LOG_WARNING(logger) << "Could not connect to server";
			status = CONNECTION_ERROR;
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			LOG_WARNING(logger) << "Unexpected ENetEvent ENET_EVENT_TYPE_RECEIVE while connecting";
			enet_packet_destroy (event.packet);
			break;
		default:
			LOG_WARNING(logger) << "Received unknown ENetEvent type << event.type";
			break;
		}
	}
}

void RemoteServerInterface::updateNetDisconnecting() {
	ENetEvent event;
	while (enet_host_service(host, &event, 0) > 0) {
		switch(event.type) {
		case ENET_EVENT_TYPE_CONNECT:
			LOG_FATAL(logger) << "Unexpected ENetEvent ENET_EVENT_TYPE_CONNECT while disconnecting";
			break;
		case ENET_EVENT_TYPE_DISCONNECT:
			peer = nullptr;
			LOG_INFO(logger) << "Disconnected from server";
			status = NOT_CONNECTED;
			break;
		case ENET_EVENT_TYPE_RECEIVE:
			LOG_WARNING(logger) << "Unexpected ENetEvent ENET_EVENT_TYPE_RECEIVE while disconnecting";
			enet_packet_destroy (event.packet);
			break;
		default:
			LOG_WARNING(logger) << "Received unknown ENetEvent type" << event.type;
			break;
		}
	}
}

void RemoteServerInterface::handlePacket(const enet_uint8 *data, size_t size, size_t channel) {
	//LOG_TRACE(logger) << "Received message of length " << size;

	MessageType type;
	if (readMessageHeader((const char *) data, size, &type)) {
		LOG_WARNING(logger) << "Received malformed message";
		return;
	}

	//LOG_TRACE(logger) << "Message type: " << (int) type;
	switch (type) {
	case PLAYER_JOIN_EVENT:
		// TODO
		break;
	case PLAYER_LEAVE_EVENT:
		// TODO
		break;
	case SNAPSHOT:
		{
			status = CONNECTED;
			Snapshot snapshot;
			if (readMessageBody((const char *) data, size, &snapshot)) {
				LOG_WARNING(logger) << "Received malformed message";
				break;
			}
			localCharacterId = snapshot.localId;
			for (int i = 0; i < MAX_CLIENTS; ++i) {
				CharacterSnapshot &characterSnapshot = *(snapshot.characterSnapshots + i);
				Character &character = client->getWorld()->getCharacter(i);
				if (characterSnapshot.valid && !character.isValid())
					client->getWorld()->addCharacter(i);
				else if (!characterSnapshot.valid && character.isValid())
					client->getWorld()->deleteCharacter(i);
				if (characterSnapshot.valid)
					character.applySnapshot(characterSnapshot, i == localCharacterId);
			}
		}
		break;
	case CHUNK_MESSAGE:
		{
			ChunkMessage msg;
			// TODO allocation should be in the chunk or something
			msg.encodedBlocks = new uint8[Chunk::SIZE];
			if (readMessageBody((const char *) data, size, &msg)) {
				LOG_WARNING(logger) << "Received malformed message";
				break;
			}
			auto it = requestedChunks.find(msg.chunkCoords);
			if (it == requestedChunks.end())
				break;
			Chunk *chunk = it->second;
			if (msg.encodedLength != 0) {
				decodeBlocks_RLE(msg.encodedBlocks, msg.encodedLength, chunk->getBlocksForInit());
				chunk->initRevision(msg.revision);
				chunk->finishInitialization();
			}
			requestedChunks.erase(it);
			receivedChunks.push(chunk);
			// TODO deallocation should also be in the chunk or something
			delete[] msg.encodedBlocks;
		}
		break;
	default:
		LOG_WARNING(logger) << "Received message of unknown type " << type;
		break;
	}
}
