#include "chunk_compression.hpp"

#include "shared/game/chunk.hpp"

#include "shared/engine/logging.hpp"

static logging::Logger logger("io");

static const uint8 ESCAPE_CHAR = (uint8) (-1);

void decodeBlocks_RLE(std::istream *is, uint8 *blocks) {
	size_t index = 0;
	while (index < Chunk::SIZE) {
		uint8 next_block;
		is->read((char *) &next_block, sizeof (uint8));

		if (next_block == ESCAPE_CHAR) {
			uint8 next_byte, block_type;
			uint32 run_length;
			is->read((char *) &next_byte, sizeof (uint8));

			// Like UTF8, the first bit signals a multi-byte sequence
			if ((next_byte & 0x80) == 0) {
				// This is the only byte
				run_length = next_byte;
			} else {
				// There is exactly one extra byte, the other 7 bits can be used for the value
				uint32 encoded_run_length = next_byte & 0x7F;
				is->read((char *) &next_byte, sizeof (uint8));
				encoded_run_length = (encoded_run_length << 8) | next_byte;
				// we count from 1 and not from 0 to save space
				run_length = encoded_run_length + 1;
			}

			is->read((char *) &block_type, sizeof (uint8));
			for (uint32 i = 0; i < run_length; ++i) {
				if (index >= Chunk::SIZE) {
					LOG_ERROR(logger) << "Block data exceeded Chunk size";
					break;
				}
				blocks[index++] = block_type;
			}
		} else {
			blocks[index++] = next_block;
		}
	}
}

int encodeBlocks_RLE(const uint8 *blocks, uint8 *buffer, size_t size) {
	uint8 cur_run_type = blocks[0];
	size_t cur_run_length = 1;
	uint8 *head = buffer;

	auto finishRun = [&]() -> int {
		if (cur_run_length == 0)
			return 0;
		if (cur_run_length > 3 || cur_run_type == ESCAPE_CHAR) {
			if (cur_run_length < 0x80) {
				if ((uint) (head - buffer) + 3u <= size) {
					*head++ = ESCAPE_CHAR;
					*head++ = cur_run_length;
					*head++ = cur_run_type;
				} else {
					return -1;
				}
			} else {
				// we count from 1 and not from 0 to save space
				size_t encoded_run_length = cur_run_length - 1;
				if ((uint) (head - buffer) + 4u <= size) {
					*head++ = ESCAPE_CHAR;
					*head++ = 0x80 | (encoded_run_length >> 8);
					*head++ = encoded_run_length & 0xFF;
					*head++ = cur_run_type;
				} else {
					return -1;
				}
			}
		} else {
			if (head - buffer + cur_run_length <= size) {
				for (size_t j = 0; j < cur_run_length; ++j)
					*head++ = cur_run_type;
			} else {
				return -1;
			}
		}
		return 0;
	};

	for (size_t i = 1; i < size; ++i) {
		uint8 next_block = blocks[i];
		if (cur_run_length >= 0x8000) {
			if (finishRun() != 0)
				return head - buffer;
			cur_run_length = 0;
		}
		if (cur_run_type == next_block) {
			cur_run_length++;
		} else {
			if (finishRun() != 0)
				return head - buffer;
			cur_run_type = next_block;
			cur_run_length = 1;
		}
	}

	finishRun();
	return head - buffer;
}

void decodeBlocks_PLAIN(std::istream *is, uint8 *blocks) {
	is->read((char *)blocks, Chunk::SIZE * sizeof (uint8));
}

int encodeBlocks_PLAIN(const uint8 *blocks, uint8 *buffer, size_t size) {
	size_t actual_size = std::min(Chunk::SIZE * sizeof(uint8), size);
	memcpy(buffer, blocks, actual_size);
	return (int) actual_size;
}