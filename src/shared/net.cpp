#include "net.hpp"

#include <cstring>

#include "shared/engine/logging.hpp"

static logging::Logger logger("net");

MessageError getMessageType(const char *data, size_t size, MessageType *type) {
	if (size < sizeof(MAGIC) + sizeof(MessageType))
		return WRONG_MESSAGE_LENGTH;
	else if (memcmp(data, MAGIC, sizeof(MAGIC)) != 0)
		return WRONG_MAGIC;
	*type = *reinterpret_cast<const MessageType *>(data + sizeof(MAGIC));
	return MESSAGE_OK;
}

template<> MessageType getMessageType(const PlayerJoinEvent &) {
	return PLAYER_JOIN_EVENT;
}

template<> MessageType getMessageType(const PlayerLeaveEvent &) {
	return PLAYER_LEAVE_EVENT;
}

template<> MessageType getMessageType(const Snapshot &) {
	return SNAPSHOT;
}

template<> MessageType getMessageType(const PlayerInput &) {
	return PLAYER_INPUT;
}
