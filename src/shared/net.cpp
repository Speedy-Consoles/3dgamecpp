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
	size_t size = HEADER_SIZE + 1;
	for (uint i = 0; i < msg.numChunks; i++) {
		size += 2;
		const ChunkRequestData &crd = msg.chunkRequestData[i];
		if (crd.cached && (crd.cachedRevision >> 3)) {
			size++;
			for (int shift = 8; shift < 32; shift += 8) {
				if (crd.cachedRevision >> shift)
					break;
				size++;
			}
		}
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
	WRITE_TYPE(msg.numChunks - 1, uint8)
	for(uint i = 0; i < msg.numChunks; i++) {
		const ChunkRequestData &crd = msg.chunkRequestData[i];
		const vec3i64 &rc = crd.relCoords;
		uint32 rev = crd.cachedRevision;
		bool cached = crd.cached;

		uint8 byte = rc[0] & 0xF;
		byte |= (rc[1] & 0xF) << 4;
		WRITE_TYPE(byte, uint8)
		byte = rc[2] & 0xF;
		if (cached) {
			if (!(rev >> 3)) {
				byte |= rev << 5;
				WRITE_TYPE(byte, uint8)
			} else {
				byte |= 0x10;
				char *infoByte = data;
				uint8 bytes = 0;
				uint32 shifted = rev;
				while(shifted) {
					WRITE_TYPE(rev & 0xFF, uint8)
					shifted = shifted >> 8;
					bytes++;
				}
				byte |= (bytes - 1) << 5;
				*reinterpret_cast<uint8 *>(infoByte) = byte;
			}
		} else {
			byte |= 0xF0;
			WRITE_TYPE(byte, uint8)
		}
	}
	return BUFFER_OK;
}
MessageError readMessageBody(const char *data, size_t size, ChunkRequest *msg) {
	if (size < HEADER_SIZE + 1)
		return ABRUPT_MESSAGE_END;
	data += HEADER_SIZE;
	size -= HEADER_SIZE;
	int numChunksMinusOne;
	READ_TYPE(numChunksMinusOne, uint8)
	msg->numChunks = numChunksMinusOne + 1;
	for (uint i = 0; i < msg->numChunks; i++) {
		ChunkRequestData &crd = msg->chunkRequestData[i];
		vec3i64 &rc = crd.relCoords;
		uint32 &rev = crd.cachedRevision;
		bool &cached = crd.cached;

		if (size < 2)
			return ABRUPT_MESSAGE_END;
		uint8 byte;
		READ_TYPE(byte, uint8)
		rc[0] = extendFourBit(byte);
		rc[1] = extendFourBit(byte >> 4);
		READ_TYPE(byte, uint8)
		rc[2] = extendFourBit(byte);
		uint8 revInfo = byte >> 4;
		if ((revInfo & 0xF) == 0xF)
			cached = false;
		else if (!(revInfo & 1)) {
			cached = true;
			rev = revInfo >> 1;
		} else {
			cached = true;
			rev = 0;
			size_t numBytes = revInfo >> 1;
			if (numBytes > 4) return MALFORMED_MESSAGE;
			if (size < numBytes) return ABRUPT_MESSAGE_END;
			int shift = 0;
			for (int j = 0; j < numBytes; j++) {
				READ_TYPE(byte, uint8)
				rev |= ((uint32) byte) << shift;
				shift += 8;
			}
		}
	}
	if (size > 0)
		return MESSAGE_TOO_LONG;

	return MESSAGE_OK;
}

// CHUNK_MESSAGE
size_t getMessageSize(const ChunkMessage &msg) {
	size_t size = HEADER_SIZE + 1;
	for (uint i = 0; i < msg.numChunks; i++) {
		const ChunkMessageData &cmd = msg.chunkMessageData[i];
		size += 4 + cmd.encodedLength;
		if (cmd.revision >> 3) {
			size++;
			for (int shift = 8; shift < 32; shift += 8) {
				if (cmd.revision >> shift)
					break;
				size++;
			}
		}
	}
	return size;
}
MessageType getMessageType(const ChunkMessage &) { return CHUNK_MESSAGE; }
BufferError writeMessage(const ChunkMessage &msg, char *data, size_t size) {
	if (size != getMessageSize(msg))
		return WRONG_BUFFER_LENGTH;
	writeHeader(CHUNK_MESSAGE, data);
	data += HEADER_SIZE;
	size -= HEADER_SIZE;
	uint8 *eb = msg.encodedBuffer;
	WRITE_TYPE(msg.numChunks - 1, uint8)
	for(uint i = 0; i < msg.numChunks; i++) {
		const ChunkMessageData &cmd = msg.chunkMessageData[i];
		const vec3i64 &rc = cmd.relCoords;
		uint32 rev = cmd.revision;
		size_t el = cmd.encodedLength;

		uint8 byte = rc[0] & 0xF;
		byte |= (rc[1] & 0xF) << 4;
		WRITE_TYPE(byte, uint8)
		byte = rc[2] & 0xF;
		if (!(rev >> 3)) {
			byte |= rev << 5;
			WRITE_TYPE(byte, uint8)
		} else {
			byte |= 0x10;
			char *infoByte = data;
			uint8 bytes = 0;
			uint32 shifted = rev;
			while(shifted) {
				WRITE_TYPE(rev & 0xFF, uint8)
				shifted = shifted >> 8;
				bytes++;
			}
			byte |= (bytes - 1) << 5;
			*reinterpret_cast<uint8 *>(infoByte) = byte;
		}
		WRITE_TYPE(el, uint16)
		memcpy(data, eb, el);
		eb += el;
		data += el;
		size -= el;
	}
	return BUFFER_OK;
}
MessageError readMessageBody(const char *data, size_t size, ChunkMessage *msg) {
	if (size < HEADER_SIZE + 1)
		return ABRUPT_MESSAGE_END;
	data += HEADER_SIZE;
	size -= HEADER_SIZE;
	uint8 *eb = msg->encodedBuffer;
	int numChunksMinusOne;
	READ_TYPE(numChunksMinusOne, uint8)
	msg->numChunks = numChunksMinusOne + 1;
	for (uint i = 0; i < msg->numChunks; i++) {
		ChunkMessageData &cmd = msg->chunkMessageData[i];
		vec3i64 &rc = cmd.relCoords;
		uint32 &rev = cmd.revision;
		size_t &el = cmd.encodedLength;

		if (size < 2)
			return ABRUPT_MESSAGE_END;
		uint8 byte;
		READ_TYPE(byte, uint8)
		rc[0] = extendFourBit(byte);
		rc[1] = extendFourBit(byte >> 4);
		READ_TYPE(byte, uint8)
		rc[2] = extendFourBit(byte);
		uint8 revInfo = byte >> 4;
		if (!(revInfo & 1)) {
			rev = revInfo >> 1;
		} else {
			rev = 0;
			size_t numBytes = revInfo >> 1;
			if (numBytes > 4) return MALFORMED_MESSAGE;
			if (size < numBytes) return ABRUPT_MESSAGE_END;
			int shift = 0;
			for (uint j = 0; j < numBytes; j++) {
				READ_TYPE(byte, uint8)
				rev |= ((uint32) byte) << shift;
				shift += 8;
			}
		}
		if (size < 2)
			return ABRUPT_MESSAGE_END;
		READ_TYPE(el, uint16)
		if (size < el)
			return ABRUPT_MESSAGE_END;
		memcpy(eb, data, el);
		eb += el;
		data += el;
		size -= el;
	}
	if (size > 0)
		return MESSAGE_TOO_LONG;
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
