#ifndef NET_HPP
#define NET_HPP

#include "engine/std_types.hpp"
#include "engine/buffer.hpp"
#include "game/player.hpp"

static const uint8 MAGIC[4] = {0xaa, 0x0d, 0xbe, 0x15};

enum MessageError : uint8 {
	MESSAGE_TOO_SHORT,
	WRONG_MAGIC,
};

enum ClientMessageType : uint8 {
	MALFORMED_CLIENT_MESSAGE,
	PLAYER_INPUT,
};

enum ServerMessageType : uint8 {
	MALFORMED_SERVER_MESSAGE,
	PLAYER_JOIN_EVENT,
	PLAYER_LEAVE_EVENT,
	SNAPSHOT,
};

struct PlayerInput {
	uint16 yaw;
	int16 pitch;
	uint8 moveInput;
	bool flying;
};

union ServerMessage {
	ServerMessageType type;
	struct { ServerMessageType type; MessageError error; } malformed;
	struct { ServerMessageType type; uint8 id; } playerJoinEvent;
	struct { ServerMessageType type; uint8 id; } playerLeaveEvent;
	struct { ServerMessageType type; uint8 id; PlayerSnapshot snapshot; } playerSnapshot;
};

union ClientMessage {
	ClientMessageType type;
	struct { ClientMessageType type; MessageError error; } malformed;
	struct { ClientMessageType type; PlayerInput input; } playerInput;
};

Buffer &operator << (Buffer &lhs, const ServerMessage &rhs);
const Buffer &operator >> (const Buffer &lhs, ServerMessage &rhs);

Buffer &operator << (Buffer &lhs, const ClientMessage &rhs);
const Buffer &operator >> (const Buffer &lhs, ClientMessage &rhs);

#endif // NET_HPP
