#ifndef BLOCK_LOADER_HPP_
#define BLOCK_LOADER_HPP_

#include <string>

class Client;

class BlockLoader {
	Client *client = nullptr;

public:
	BlockLoader(Client *client) : client(client) {}
	~BlockLoader() = default;

	void add(std::string key, int value);
};

#endif //BLOCK_LOADER_HPP_
