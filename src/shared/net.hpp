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
	int id;
};

struct PlayerLeaveEvent {
	int id;
};

struct PlayerSnapshot {
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
	PlayerSnapshot playerSnapshots[MAX_CLIENTS];
	int localId;
};

struct PlayerInput {
	int yaw;
	int pitch;
	int moveInput;
	bool flying;
};

MessageError readMessageHeader(const char *data, size_t size, MessageType *type);

#define MSG_FUNCS(msg_name) \
	size_t getMessageSize(const msg_name &); \
	BufferError writeMessage(const msg_name &msg, char *data, size_t size); \
	MessageError readMessageBody(const char *data, size_t size, msg_name *msg);

MSG_FUNCS(PlayerJoinEvent)
MSG_FUNCS(PlayerLeaveEvent)
MSG_FUNCS(Snapshot)
MSG_FUNCS(PlayerInput)

#endif // NET_HPP
