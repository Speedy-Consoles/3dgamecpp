#ifndef BLOCK_MANAGER_HPP_
#define BLOCK_MANAGER_HPP_

#include "engine/std_types.hpp"

#include <string>
#include <map>
#include <vector>

class BlockLoader;

class BlockManager {
public:
	struct Entry {
		int id;
	};

	int load(const char *file);
	int getBlockNumber() const { return entries.size(); }
	int getBlockId(std::string name) const {
		return entries.count(name) > 0 ? entries.at(name).id : -1;
	}
	const std::map<std::string, Entry> &getBlocks() const { return entries; }

private:
	friend BlockLoader;
	std::map<std::string, Entry> entries;
};

#endif //BLOCK_MANAGER_HPP_