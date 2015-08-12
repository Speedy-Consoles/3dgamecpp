#ifndef TEXTURE_MANAGER_HPP_
#define TEXTURE_MANAGER_HPP_

#include <vector>

#include "texture_loader.hpp"

class TextureManager {
protected:
	Client *_client = nullptr;
	std::vector<std::string> files;

public:
	virtual ~TextureManager() = default;
	TextureManager(Client *client) : _client(client) {}
	int load(const char *);
	int reloadAll();

protected:
	friend TextureLoader;
	virtual void add(SDL_Surface *img, const std::vector<TextureLoadEntry> &entries) = 0;
	virtual void clear() = 0;
};

#endif // TEXTURE_MANAGER_HPP_
