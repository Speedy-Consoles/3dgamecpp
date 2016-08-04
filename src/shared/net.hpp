#ifndef NET_HPP
#define NET_HPP

#include <cstring>
#include <string>
#include <sstream>

#include "constants.hpp"

#include "engine/vmath.hpp"
#include "engine/std_types.hpp"

enum ChannelNames {
	CHANNEL_STATE = 0,
	CHANNEL_BLOCK_DATA,
	CHANNEL_CHAT,
	NUM_CHANNELS,
};

enum DisconnectReason : uint32 {
	TIMEOUT = 0,
	CLIENT_LEAVE,
	KICKED,
	SERVER_SHUTDOWN,
};

enum MessageError : uint8 {
	MESSAGE_OK = 0,
	ABRUPT_MESSAGE_END,
	MESSAGE_TOO_LONG,
	MALFORMED_MESSAGE,
};

enum BufferError : uint8 {
	BUFFER_OK = 0,
	WRONG_BUFFER_LENGTH,
};

enum MessageType : uint8 {
	UNKNOWN_MESSAGE_TYPE,

	// Server messages
	PLAYER_JOIN_EVENT,
	PLAYER_LEAVE_EVENT,
	SNAPSHOT,
	CHUNK_MESSAGE,

	// Client messages
	PLAYER_INFO,
	PLAYER_INPUT,
	CHUNK_REQUEST,

	// Client/Server
	CHUNK_ANCHOR_SET,
};

struct PlayerJoinEvent {
	int id;
};

struct PlayerLeaveEvent {
	int id;
};

struct CharacterSnapshot {
	bool valid;
	vec3i64 pos;
	vec3d vel;
	int yaw;
	int pitch;
	int moveInput;

	bool isFlying;
};

struct Snapshot {
	int tick;
	CharacterSnapshot characterSnapshots[MAX_CLIENTS];
	int localId;
};

struct ChunkMessageData {
	vec3i64 relCoords;
	uint32 revision;
	size_t encodedLength;
};

const size_t MAX_CHUNKS_PER_MESSAGE = 256;
struct ChunkMessage {
	uint numChunks;
	ChunkMessageData chunkMessageData[MAX_CHUNKS_PER_MESSAGE];
	uint8 *encodedBuffer;
};

struct PlayerInfo {
	std::string name;
};

struct PlayerInput {
	int yaw;
	int pitch;
	int moveInput;
	bool flying;
};

struct ChunkRequestData {
	vec3i64 relCoords;
	bool cached;
	uint32 cachedRevision;
};

const size_t MAX_CHUNKS_PER_REQUEST = 256;
struct ChunkRequest {
	uint numChunks;
	ChunkRequestData chunkRequestData[MAX_CHUNKS_PER_REQUEST];
};

struct ChunkAnchorSet {
	vec3i64 coords;
};

MessageError readMessageHeader(const char *data, size_t size, MessageType *type);

#define MSG_FUNCS(msg_name) \
	size_t getMessageSize(const msg_name &); \
	BufferError writeMessage(const msg_name &msg, char *data, size_t size); \
	MessageError readMessageBody(const char *data, size_t size, msg_name *msg);

MSG_FUNCS(PlayerJoinEvent)
MSG_FUNCS(PlayerLeaveEvent)
MSG_FUNCS(Snapshot)
MSG_FUNCS(PlayerInfo)
MSG_FUNCS(PlayerInput)
MSG_FUNCS(ChunkRequest)
MSG_FUNCS(ChunkMessage)
MSG_FUNCS(ChunkAnchorSet)

#endif // NET_HPP
