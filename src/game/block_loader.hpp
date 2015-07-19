#ifndef BLOCK_LOADER_HPP_
#define BLOCK_LOADER_HPP_

#include <string>

class BlockLoader;

class AbstractBlockManager {
public:
	int load(const char *);

protected:
	friend BlockLoader;
	virtual void add(std::string key, int value) = 0;
};

#endif //BLOCK_LOADER_HPP_
