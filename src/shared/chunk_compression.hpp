#ifndef CHUNK_COMPRESSION_HPP_
#define CHUNK_COMPRESSION_HPP_

#include <istream>

#include "shared/engine/std_types.hpp"

void decodeBlocks_RLE(std::istream *is, uint8 *blocks);
int encodeBlocks_RLE(const uint8 *blocks, uint8 *, size_t);
void decodeBlocks_PLAIN(std::istream *is, uint8 *blocks);
int encodeBlocks_PLAIN(const uint8 *blocks, uint8 *, size_t);

#endif // CHUNK_COMPRESSION_HPP_
