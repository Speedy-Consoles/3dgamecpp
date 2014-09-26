#include "net.hpp"

#include <cstring>

bool checkMagic(const MessageHeader &header) {
	return memcmp(header.magic, MAGIC, sizeof (header.magic)) == 0;
}
