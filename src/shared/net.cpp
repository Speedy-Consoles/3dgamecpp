#include "net.hpp"

#include <cstring>

#include "shared/engine/logging.hpp"

static logging::Logger logger("net");

static const size_t HEADER_SIZE = sizeof(MAGIC) + sizeof(MessageType);

static void writeHeader(MessageType type, char *data) {
	memcpy(data, MAGIC, sizeof(MAGIC));
	data += sizeof(MAGIC);
	*reinterpret_cast<MessageType *>(data) = type;
}

MessageError readMessageHeader(const char *data, size_t size, MessageType *type) {
	if (size < sizeof(MAGIC) + sizeof(MessageType))
		return ABRUPT_MESSAGE_END;
	else if (memcmp(data, MAGIC, sizeof(MAGIC)) != 0)
		return WRONG_MAGIC;
	*type = *reinterpret_cast<const MessageType *>(data + sizeof(MAGIC));
	return MESSAGE_OK;
}

// TODO (resize packet when serializing?)

#define PLAIN_MSG_START(msg_camel, msg_caps, msg_body_size) \
	size_t getMessageSize(const msg_camel &) { return HEADER_SIZE + msg_body_size; } \
	MessageType getMessageType(const msg_camel &) { return msg_caps; } \
	BufferError writeMessage(const msg_camel &msg, char *data, size_t size) { \
		if (size != HEADER_SIZE + msg_body_size) \
			return WRONG_BUFFER_LENGTH; \
		writeHeader(msg_caps, data); \
		data += HEADER_SIZE; \
		size -= HEADER_SIZE;

#define PLAIN_MSG_MIDDLE(msg_camel, msg_body_size) \
		return BUFFER_OK; \
	} \
	MessageError readMessageBody(const char *data, size_t size, msg_camel *msg) { \
		if (size < HEADER_SIZE + msg_body_size) \
			return ABRUPT_MESSAGE_END; \
		else if (size > HEADER_SIZE + msg_body_size) \
			return MESSAGE_TOO_LONG; \
		data += HEADER_SIZE; \
		size -= HEADER_SIZE;

#define PLAIN_MSG_END \
		return MESSAGE_OK; \
	}

#define READ_TYPE(value, type) { \
	value = *reinterpret_cast<const type *>(data); \
	data += sizeof(type); \
	size -= sizeof(type); \
}

#define WRITE_TYPE(value, type) { \
	*reinterpret_cast<type *>(data) = value; \
	data += sizeof(type); \
	size -= sizeof(type); \
}

// PLAYER_JOIN_EVENT
static const size_t PLAYER_JOIN_EVENT_SIZE = sizeof(uint8);
PLAIN_MSG_START(PlayerJoinEvent, PLAYER_JOIN_EVENT, PLAYER_JOIN_EVENT_SIZE)
	WRITE_TYPE(msg.id, uint8)
PLAIN_MSG_MIDDLE(PlayerJoinEvent, PLAYER_JOIN_EVENT_SIZE)
	READ_TYPE(msg->id, uint8)
PLAIN_MSG_END

// PLAYER_LEAVE_EVENT
static const size_t PLAYER_LEAVE_EVENT_SIZE = sizeof(uint8);
PLAIN_MSG_START(PlayerLeaveEvent, PLAYER_LEAVE_EVENT, PLAYER_LEAVE_EVENT_SIZE)
	WRITE_TYPE(msg.id, uint8)
PLAIN_MSG_MIDDLE(PlayerLeaveEvent, PLAYER_LEAVE_EVENT_SIZE)
	READ_TYPE(msg->id, uint8)
PLAIN_MSG_END

// SNAPSHOT
static const size_t SNAPSHOT_SIZE =
		sizeof(uint32)
		+ MAX_CLIENTS * (
			sizeof(uint8)
			+ 3 * sizeof(int64)
			+ 3 * sizeof(double)
			+ sizeof(uint16)
			+ sizeof(int16)
			+ sizeof(uint8)
		)
		+ sizeof(uint8);
enum PlayerSnapshotFlags : uint8 {
	SNAPSHOT_FLAG_VALID = 1,
	SNAPSHOT_FLAG_IS_FLYING = 2,
};
PLAIN_MSG_START(Snapshot, SNAPSHOT, SNAPSHOT_SIZE)
	WRITE_TYPE(msg.tick, uint32)
	for (int i = 0; i < MAX_CLIENTS; i++) {
		uint8 flags = 0;
		if (msg.characterSnapshots[i].valid)
			flags |= SNAPSHOT_FLAG_VALID;
		if (msg.characterSnapshots[i].isFlying)
			flags |= SNAPSHOT_FLAG_IS_FLYING;
		WRITE_TYPE(flags, uint8)
		for (int j = 0; j < 3; j++)
			WRITE_TYPE(msg.characterSnapshots[i].pos[j], int64)
		for (int j = 0; j < 3; j++)
			WRITE_TYPE(msg.characterSnapshots[i].vel[j], double)
		WRITE_TYPE(msg.characterSnapshots[i].yaw, uint16)
		WRITE_TYPE(msg.characterSnapshots[i].pitch, int16)
		WRITE_TYPE(msg.characterSnapshots[i].moveInput, uint8)
	}
	WRITE_TYPE(msg.localId, uint8);
PLAIN_MSG_MIDDLE(Snapshot, SNAPSHOT_SIZE)
	READ_TYPE(msg->tick, uint32);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		uint8 flags;
		READ_TYPE(flags, uint8)
		msg->characterSnapshots[i].valid = flags & SNAPSHOT_FLAG_VALID;
		msg->characterSnapshots[i].isFlying = flags & SNAPSHOT_FLAG_IS_FLYING;
		for (int j = 0; j < 3; j++)
			READ_TYPE(msg->characterSnapshots[i].pos[j], int64)
		for (int j = 0; j < 3; j++)
			READ_TYPE(msg->characterSnapshots[i].vel[j], double)
		READ_TYPE(msg->characterSnapshots[i].yaw, uint16)
		READ_TYPE(msg->characterSnapshots[i].pitch, int16)
		READ_TYPE(msg->characterSnapshots[i].moveInput, uint8)
	}
	READ_TYPE(msg->localId, uint8)
PLAIN_MSG_END

// PLAYER_INFO
size_t getMessageSize(const PlayerInfo &msg) { return HEADER_SIZE + msg.name.size() + 1; }
MessageType getMessageType(const PlayerInfo &) { return PLAYER_INFO; }
BufferError writeMessage(const PlayerInfo &msg, char *data, size_t size) {
	if (size != getMessageSize(msg))
		return WRONG_BUFFER_LENGTH;
	writeHeader(PLAYER_INFO, data);
	data += HEADER_SIZE;
	size -= HEADER_SIZE;
	msg.name.copy(data, msg.name.size());
	data += msg.name.size();
	size -= msg.name.size();
	*data = '\0';
	data += sizeof(char);
	size -= sizeof(char);
	return BUFFER_OK;
}
MessageError readMessageBody(const char *data, size_t size, PlayerInfo *msg) {
	if (size < HEADER_SIZE + 1)
		return ABRUPT_MESSAGE_END;
	data += HEADER_SIZE;
	size -= HEADER_SIZE;
	char *pc = (char *) std::memchr(data, '\0', size);
	if (!pc)
		return ABRUPT_MESSAGE_END;
	size_t nameLength = pc - data;
	if (nameLength + 1 < size)
		return MESSAGE_TOO_LONG;
	msg->name = std::string(data);
	data = pc + 1;
	size -= nameLength + 1;
	return MESSAGE_OK;
}

// PLAYER_INPUT
static const size_t PLAYER_INPUT_SIZE =
		sizeof(uint8)
		+ sizeof(uint16)
		+ sizeof(int16)
		+ sizeof(uint8);
enum PlayerInputFlags : uint8 {
	INPUT_FLAG_IS_FLYING = 1,
};
PLAIN_MSG_START(PlayerInput, PLAYER_INPUT, PLAYER_INPUT_SIZE)
	uint8 flags = 0;
	if (msg.flying)
		flags |= INPUT_FLAG_IS_FLYING;
	WRITE_TYPE(flags, uint8)
	WRITE_TYPE(msg.yaw, uint16)
	WRITE_TYPE(msg.pitch, int16)
	WRITE_TYPE(msg.moveInput, uint8)
PLAIN_MSG_MIDDLE(PlayerInput, PLAYER_INPUT_SIZE)
	uint8 flags;
	READ_TYPE(flags, uint8);
	msg->flying = (flags & INPUT_FLAG_IS_FLYING) != 0;
	READ_TYPE(msg->yaw, uint16);
	READ_TYPE(msg->pitch, int16);
	READ_TYPE(msg->moveInput, uint8);
PLAIN_MSG_END

// CHUNK_REQUEST
static const size_t CHUNK_REQUEST_SIZE =
		3 * sizeof(int64)
		+ sizeof(uint32);
PLAIN_MSG_START(ChunkRequest, CHUNK_REQUEST, CHUNK_REQUEST_SIZE)
	for (int i = 0; i < 3; i++)
		WRITE_TYPE(msg.coords[i], int64)
	if (msg.cached)
		WRITE_TYPE((uint32) -1, uint32)
	else
		WRITE_TYPE(msg.cachedRevision, uint32)
PLAIN_MSG_MIDDLE(ChunkRequest, CHUNK_REQUEST_SIZE)
	for (int i = 0; i < 3; i++)
		READ_TYPE(msg->coords[i], int64)
	uint32 rev;
	READ_TYPE(rev, uint32)
	if (rev == (uint32) -1) {
		msg->cached = true;
		msg->cachedRevision = rev;
	} else {
		msg->cached = false;
	}
PLAIN_MSG_END

// CHUNK_MESSAGE
static const size_t CHUNK_MESSAGE_META_SIZE =
		sizeof(int64) * 3
		+ sizeof(uint32)
		+ sizeof(uint16);
size_t getMessageSize(const ChunkMessage &msg) {
	return HEADER_SIZE + CHUNK_MESSAGE_META_SIZE + msg.encodedLength;
}
MessageType getMessageType(const ChunkMessage &) { return CHUNK_MESSAGE; }
char *getEncodedBlocksPointer(char *data) {
	return data + HEADER_SIZE + CHUNK_MESSAGE_META_SIZE;
}
BufferError writeMessageMeta(const ChunkMessage &msg, char *data, size_t size) {
	if (size != getMessageSize(msg))
		return WRONG_BUFFER_LENGTH;
	writeHeader(PLAYER_INFO, data);
	data += HEADER_SIZE;
	size -= HEADER_SIZE;
	for (int i = 0; i < 3; i++)
		WRITE_TYPE(msg.chunkCoords[i], int64)
	WRITE_TYPE(msg.revision, uint32)
	WRITE_TYPE(msg.encodedLength, uint16)
	// after this come the already written encoded blocks
	return BUFFER_OK;
}
MessageError readMessageBody(const char *data, size_t size, ChunkMessage *msg) {
	if (size < HEADER_SIZE + CHUNK_MESSAGE_META_SIZE)
		return ABRUPT_MESSAGE_END;
	data += HEADER_SIZE;
	size -= HEADER_SIZE;
	for (int i = 0; i < 3; i++)
		READ_TYPE(msg->chunkCoords[i], int64)
	READ_TYPE(msg->revision, uint32)
	READ_TYPE(msg->encodedLength, uint16)
	if (size < msg->encodedLength)
		return ABRUPT_MESSAGE_END;
	msg->encodedBlocks = data;
	return MESSAGE_OK;
}
