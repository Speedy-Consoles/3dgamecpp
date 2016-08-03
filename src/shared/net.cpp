#include "net.hpp"

#include <cstring>

#include "shared/engine/logging.hpp"

static logging::Logger logger("net");

static const size_t HEADER_SIZE = sizeof(MessageType);

static int8 extendFourBit(uint8 src) {
	int8 sign = (src & 0x8) >> 3;
	int8 value = (src & 0x7);
	if (sign)
		value = -8 + value;
	return value;
}

static void writeHeader(MessageType type, char *data) {
	*reinterpret_cast<MessageType *>(data) = type;
}

MessageError readMessageHeader(const char *data, size_t size, MessageType *type) {
	if (size < sizeof(MessageType))
		return ABRUPT_MESSAGE_END;
	*type = *reinterpret_cast<const MessageType *>(data);
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
		msg->characterSnapshots[i].isFlying = (flags & SNAPSHOT_FLAG_IS_FLYING) != 0;
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
size_t getMessageSize(const ChunkRequest &msg) {
	if (!msg.cached || !(msg.cachedRevision >> 3))
		return HEADER_SIZE + 2;
	size_t size = HEADER_SIZE + 3;
	for (int shift = 8; shift < 32; shift += 8) {
		if (msg.cachedRevision >> shift)
			break;
		size++;
	}
	return size;
}
MessageType getMessageType(const ChunkRequest &) { return CHUNK_REQUEST; }
BufferError writeMessage(const ChunkRequest &msg, char *data, size_t size) {
	if (size != getMessageSize(msg))
		return WRONG_BUFFER_LENGTH;
	writeHeader(CHUNK_REQUEST, data);
	data += HEADER_SIZE;
	size -= HEADER_SIZE;
	uint8 byte = msg.relCoords[0] & 0xF;
	byte |= (msg.relCoords[1] & 0xF) << 4;
	WRITE_TYPE(byte, uint8)
	byte = msg.relCoords[2] & 0xF;
	if (msg.cached) {
		if (size == 1) {
			byte |= msg.cachedRevision << 5;
			WRITE_TYPE(byte, uint8)
		} else {
			byte |= 0x10;
			byte |= (size - 1) << 5;
			WRITE_TYPE(byte, uint8)
			int shift = 0;
			while (size > 0) {
				byte = (msg.cachedRevision >> shift) & 0xFF;
				WRITE_TYPE(byte, uint8)
				shift += 8;
			}
		}
	} else {
		byte |= 0xF0;
		WRITE_TYPE(byte, uint8)
	}
	return BUFFER_OK;
}
MessageError readMessageBody(const char *data, size_t size, ChunkRequest *msg) {
	if (size < HEADER_SIZE + 2)
		return ABRUPT_MESSAGE_END;
	data += HEADER_SIZE;
	size -= HEADER_SIZE;
	uint8 byte;
	READ_TYPE(byte, uint8)
	msg->relCoords[0] = extendFourBit(byte);
	msg->relCoords[1] = extendFourBit(byte >> 4);
	READ_TYPE(byte, uint8)
	msg->relCoords[2] = extendFourBit(byte);
	uint8 revInfo = byte >> 4;
	if ((revInfo & 0xF) == 0xF)
		msg->cached = false;
	else if (!(revInfo & 1)) {
		msg->cached = true;
		msg->cachedRevision = revInfo >> 1;
	} else {
		msg->cached = true;
		msg->cachedRevision = 0;
		size_t numBytes = revInfo >> 1;
		if (numBytes > 4) return MALFORMED_MESSAGE;
		if (size < numBytes) return ABRUPT_MESSAGE_END;
		if (size > numBytes) return MESSAGE_TOO_LONG;
		int shift = 0;
		while (size > 0) {
			READ_TYPE(byte, uint8)
			msg->cachedRevision |= ((uint32) byte) << shift;
			shift += 8;
		}
	}

	return MESSAGE_OK;
}

// CHUNK_MESSAGE
static const size_t CHUNK_MESSAGE_META_SIZE =
		sizeof(int64) * 3
		+ sizeof(uint32)
		+ sizeof(uint16);
size_t getMessageSize(const ChunkMessage &msg) {
	return HEADER_SIZE + CHUNK_MESSAGE_META_SIZE + msg.encodedLength;
}
MessageType getMessageType(const ChunkMessage &) { return CHUNK_MESSAGE; }
BufferError writeMessage(const ChunkMessage &msg, char *data, size_t size) {
	if (size != getMessageSize(msg))
		return WRONG_BUFFER_LENGTH;
	writeHeader(CHUNK_MESSAGE, data);
	data += HEADER_SIZE;
	size -= HEADER_SIZE;
	for (int i = 0; i < 3; i++)
		WRITE_TYPE(msg.chunkCoords[i], int64)
	WRITE_TYPE(msg.revision, uint32)
	WRITE_TYPE(msg.encodedLength, uint16)
	memcpy(data, msg.encodedBlocks, msg.encodedLength);
	data += msg.encodedLength;
	size -= msg.encodedLength;
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
	memcpy(msg->encodedBlocks, data, msg->encodedLength);
	data += msg->encodedLength;
	size -= msg->encodedLength;
	return MESSAGE_OK;
}

// CHUNK_ANCHOR_SET
static const size_t CHUNK_ANCHOR_SET_SIZE = sizeof(int64) * 3;
PLAIN_MSG_START(ChunkAnchorSet, CHUNK_ANCHOR_SET, CHUNK_ANCHOR_SET_SIZE)
	WRITE_TYPE(msg.coords[0], int64)
	WRITE_TYPE(msg.coords[1], int64)
	WRITE_TYPE(msg.coords[2], int64)
PLAIN_MSG_MIDDLE(ChunkAnchorSet, CHUNK_ANCHOR_SET_SIZE)
	READ_TYPE(msg->coords[0], int64)
	READ_TYPE(msg->coords[1], int64)
	READ_TYPE(msg->coords[2], int64)
PLAIN_MSG_END
