#include "net.hpp"

#include <cstring>

#include "shared/engine/logging.hpp"

static logging::Logger logger("net");

Buffer &operator << (Buffer &lhs, const ServerMessage &rhs) {
	switch (rhs.type) {
	case PLAYER_JOIN_EVENT:
		return lhs << MAGIC << rhs.playerJoinEvent;
	case PLAYER_LEAVE_EVENT:
		return lhs << MAGIC << rhs.playerLeaveEvent;
	case SNAPSHOT:
		return lhs << MAGIC << rhs.playerSnapshot;
	default:
		LOG_ERROR(logger) << "Tried to buffer server message of unknown type";
		return lhs;
	}
}

const Buffer &operator >> (const Buffer &lhs, ServerMessage &rhs) {
	if (lhs.rSize() < (sizeof (MAGIC)) + sizeof (uint8)) {
		rhs.type = MALFORMED_SERVER_MESSAGE;
		rhs.malformed.error = MESSAGE_TOO_SHORT;
	} else if (memcmp(lhs.rBegin(), MAGIC, sizeof (MAGIC)) != 0) {
		rhs.type = MALFORMED_SERVER_MESSAGE;
		rhs.malformed.error = WRONG_MAGIC;
	} else {
		lhs.rSeekRel(sizeof (MAGIC));
		uint8 type = *lhs.rBegin();
		const char *oldBegin = lhs.rBegin();
		switch (type) {
		case PLAYER_JOIN_EVENT:
			lhs >> rhs.playerJoinEvent;
			break;
		case PLAYER_LEAVE_EVENT:
			lhs >> rhs.playerLeaveEvent;
			break;
		case SNAPSHOT:
			lhs >> rhs.playerSnapshot;
			break;
		default:
			LOG_ERROR(logger) << "Buffer contains server message of unknown type";
			break;
		}
		if (lhs.rBegin() == oldBegin) {
			rhs.type = MALFORMED_SERVER_MESSAGE;
			rhs.malformed.error = MESSAGE_TOO_SHORT;
		}
	}
	return lhs;
}

Buffer &operator << (Buffer &lhs, const ClientMessage &rhs) {
	switch (rhs.type) {
	case PLAYER_INPUT:
		return lhs << MAGIC << rhs.playerInput;
	default:
		LOG_ERROR(logger) << "Tried to buffer client message of unknown type";
		return lhs;
	}
}

const Buffer &operator >> (const Buffer &lhs, ClientMessage &rhs) {
	if (lhs.rSize() < (sizeof (MAGIC)) + sizeof (uint8)) {
		rhs.type = MALFORMED_CLIENT_MESSAGE;
		rhs.malformed.error = MESSAGE_TOO_SHORT;
	} else if (memcmp(lhs.rBegin(), MAGIC, sizeof (MAGIC)) != 0) {
		rhs.type = MALFORMED_CLIENT_MESSAGE;
		rhs.malformed.error = WRONG_MAGIC;
	} else {
		lhs.rSeekRel(sizeof (MAGIC));
		uint8 type = *lhs.rBegin();
		const char *oldBegin = lhs.rBegin();
		switch (type) {
		case PLAYER_INPUT:
			lhs >> rhs.playerInput;
			break;
		default:
			LOG_ERROR(logger) << "Buffer contains client message of unknown type";
			break;
		}
		if (lhs.rBegin() == oldBegin) {
			rhs.type = MALFORMED_CLIENT_MESSAGE;
			rhs.malformed.error = MESSAGE_TOO_SHORT;
		}
	}
	return lhs;
}
