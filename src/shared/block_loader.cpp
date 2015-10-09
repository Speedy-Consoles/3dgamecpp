#include "block_loader.hpp"

#include "shared/block_manager.hpp"

#include "client/client.hpp"

void BlockLoader::add(std::string key, int value) {
	client->getBlockManager()->add(key, value);
}