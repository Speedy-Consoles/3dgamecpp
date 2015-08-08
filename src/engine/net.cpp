#include "net.hpp"

#include <cstring>
#include "logging.hpp"

#include "engine/logging.hpp"
static logging::Logger logger("net");

Buffer &operator << (Buffer &lhs, const ServerMessage &rhs) {
	switch (rhs.type) {
	case CONNECTION_ACCEPTED:
		return lhs << MAGIC << rhs.conAccepted;
	case CONNECTION_REJECTED:
		return lhs << MAGIC << rhs.conRejected;
	case CONNECTION_TIMEOUT:
		return lhs << MAGIC << rhs.conTimeout;
	case CONNECTION_RESET:
		return lhs << MAGIC << rhs.conReset;
	case ECHO_RESPONSE:
		return lhs << MAGIC << rhs.echoResp;
	case PLAYER_SNAPSHOT:
		return lhs << MAGIC << rhs.playerSnapshot;
	default:
		LOG_ERROR(logger) << "Tried to send server message of unknown type";
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
		case CONNECTION_ACCEPTED:
			lhs >> rhs.conAccepted;
			break;
		case CONNECTION_REJECTED:
			lhs >> rhs.conRejected;
			break;
		case CONNECTION_TIMEOUT:
			lhs >> rhs.conTimeout;
			break;
		case CONNECTION_RESET:
			lhs >> rhs.conReset;
			break;
		case ECHO_RESPONSE:
			lhs >> rhs.echoResp;
			break;
		case PLAYER_SNAPSHOT:
			lhs >> rhs.playerSnapshot;
			break;
		default:
			LOG_ERROR(logger) << "Received server message of unknown type";
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
	case CONNECTION_REQUEST:
		return lhs << MAGIC << rhs.conRequest;
	case ECHO_REQUEST:
		return lhs << MAGIC << rhs.echoRequest;
	case PLAYER_INPUT:
		return lhs << MAGIC << rhs.playerInput;
	default:
		LOG_ERROR(logger) << "Tried to send client message of unknown type";
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
		case CONNECTION_REQUEST:
			lhs >> rhs.conRequest;
			break;
		case ECHO_REQUEST:
			lhs >> rhs.echoRequest;
			break;
		case PLAYER_INPUT:
			lhs >> rhs.playerInput;
			break;
		default:
			LOG_ERROR(logger) << "Received client message of unknown type";
			break;
		}
		if (lhs.rBegin() == oldBegin) {
			rhs.type = MALFORMED_CLIENT_MESSAGE;
			rhs.malformed.error = MESSAGE_TOO_SHORT;
		}
	}
	return lhs;
}
