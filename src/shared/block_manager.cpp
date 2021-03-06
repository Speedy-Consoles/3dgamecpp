#include "block_manager.hpp"

#include <vector>
#include <string>

int BlockManager::getBlockId(std::string name) const {
	return entries.count(name) > 0 ? entries.at(name).id : -1;
}

const std::map<std::string, BlockManager::Entry> &BlockManager::getBlocks() const {
	return entries;
}

void BlockManager::add(std::string key, int value) {
	BlockManager::Entry entry{value};
	std::pair<std::string, BlockManager::Entry> pair(key, entry);
	entries.insert(pair);
	if (value > 0)
		++entry_count;
}
