#ifndef NET_HPP
#define NET_HPP

#include <cstring>

#include "constants.hpp"

#include "engine/vmath.hpp"
#include "engine/std_types.hpp"
#include "engine/buffer.hpp"

static const uint8 MAGIC[4] = {0xaa, 0x0d, 0xbe, 0x15};

enum MessageError : uint8 {
	MESSAGE_OK = 0,
	WRONG_MESSAGE_LENGTH,
	WRONG_MAGIC,
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

	// Client messages
	PLAYER_INPUT,
};

struct PlayerJoinEvent {
	uint8 id;
};

struct PlayerLeaveEvent {
	uint8 id;
};

struct PlayerSnapshot {
	bool valid;
	vec3i64 pos;
	vec3d vel;
	uint16 yaw;
	int16 pitch;
	int moveInput;
	bool isFlying;
};

struct Snapshot {
	int tick;
	PlayerSnapshot playerSnapshots[MAX_CLIENTS];
	uint8 localId;
};

struct PlayerInput {
	uint16 yaw;
	int16 pitch;
	uint8 moveInput;
	bool flying;
};

template<typename T> size_t getMessageSize(const T &) {
	// TODO consider padding
	return sizeof(MAGIC) + sizeof(MessageType) + sizeof(T);
}

MessageError getMessageType(const char *data, size_t size, MessageType *type);

template<typename T> MessageType getMessageType(T &);
template<> MessageType getMessageType(PlayerJoinEvent&);
template<> MessageType getMessageType(PlayerLeaveEvent &);
template<> MessageType getMessageType(Snapshot &);
template<> MessageType getMessageType(PlayerInput &);

template<typename T> MessageType getMessageType(const T &) {
	return UNKNOWN_MESSAGE_TYPE;
}

template<typename T> BufferError serialize(const T &msg, char *data, size_t size) {
	// TODO (resize?)
	if (size != getMessageSize(msg))
		return WRONG_BUFFER_LENGTH;
	memcpy(data, MAGIC, sizeof(T));
	data += sizeof(MAGIC);
	*reinterpret_cast<MessageType *>(data) = getMessageType(msg);
	data += sizeof(MessageType);
	memcpy(data, reinterpret_cast<const char *>(&msg), sizeof(T));
	return BUFFER_OK;
}

template<typename T> MessageError deserialize(const char *data, size_t size, T *msg) {
	if (size != getMessageSize(msg))
		return WRONG_MESSAGE_LENGTH;
	memcpy(reinterpret_cast<char *>(msg), data, sizeof(T));
	return MESSAGE_OK;
}

#endif // NET_HPP
