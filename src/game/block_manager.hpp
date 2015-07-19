#ifndef BLOCK_MANAGER_HPP_
#define BLOCK_MANAGER_HPP_

#include "block_loader.hpp"

#include "engine/std_types.hpp"

#include <string>
#include <map>
#include <vector>

class BlockManager : public AbstractBlockManager {
public:
	struct Entry {
		int id;
	};

	int getBlockNumber() const;
	int getBlockId(std::string name) const;
	const std::map<std::string, Entry> &getBlocks() const;

protected:
	void add(std::string, int) override;

private:
	std::map<std::string, Entry> entries;
};

#endif //BLOCK_MANAGER_HPP_