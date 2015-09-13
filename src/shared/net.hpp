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
	CONNECTION_REQUEST,
	ECHO_REQUEST,
	PLAYER_INPUT,
};

enum ServerMessageType : uint8 {
	MALFORMED_SERVER_MESSAGE,
	CONNECTION_ACCEPTED,
	CONNECTION_REJECTED,
	CONNECTION_TIMEOUT,
	CONNECTION_RESET,
	ECHO_RESPONSE,
	PLAYER_JOIN,
	PLAYER_LEAVE,
	PLAYER_SNAPSHOT,
};

enum RejectionReason : uint8 {
	FULL,
	DUPLICATE_ENDPOINT,
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
	struct { ServerMessageType type; uint8 id; } conAccepted;
	struct { ServerMessageType type; RejectionReason reason; } conRejected;
	struct { ServerMessageType type; } conTimeout;
	struct { ServerMessageType type; } conReset;
	struct { ServerMessageType type; } echoResp;
	struct { ServerMessageType type; uint8 id; } playerJoin;
	struct { ServerMessageType type; uint8 id; } playerLeave;
	struct { ServerMessageType type; uint8 id; PlayerSnapshot snapshot; } playerSnapshot;
};

union ClientMessage {
	ClientMessageType type;
	struct { ClientMessageType type; MessageError error; } malformed;
	struct { ClientMessageType type; } conRequest;
	struct { ClientMessageType type; } echoRequest;
	struct { ClientMessageType type; PlayerInput input; } playerInput;
};

Buffer &operator << (Buffer &lhs, const ServerMessage &rhs);
const Buffer &operator >> (const Buffer &lhs, ServerMessage &rhs);

Buffer &operator << (Buffer &lhs, const ClientMessage &rhs);
const Buffer &operator >> (const Buffer &lhs, ClientMessage &rhs);

#endif // NET_HPP
