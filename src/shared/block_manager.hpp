#ifndef BLOCK_MANAGER_HPP_
#define BLOCK_MANAGER_HPP_

#include "engine/std_types.hpp"

#include <string>
#include <map>
#include <vector>

class BlockLoader;

class BlockManager {
	int entry_count = 0;
	struct Entry {
		int id;
	};
	std::map<std::string, Entry> entries;

public:
	int getNumberOfBlocks() const { return entry_count; }
	int getBlockId(std::string name) const;
	const std::map<std::string, Entry> &getBlocks() const;

private:
	friend BlockLoader;
	void add(std::string, int);
};

#endif //BLOCK_MANAGER_HPP_