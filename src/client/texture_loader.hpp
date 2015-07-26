#ifndef TEXTURE_LOADER_HPP_
#define TEXTURE_LOADER_HPP_

#include <SDL2/SDL.h>

#include <string>
#include <vector>

class TextureLoader;
class Client;

enum class TextureType {
	SINGLE_TEXTURE,
	WANG_TILES,
	MULTI_x4,
};

struct TextureEntry {
	int id;
	TextureType type;
	int x, y, w, h;
};

class AbstractTextureManager {
protected:
	Client *_client = nullptr;
	std::vector<std::string> files;

public:
	virtual ~AbstractTextureManager() = default;
	AbstractTextureManager(Client *client) : _client(client) {}
	int load(const char *);
	int reloadAll();

protected:
	friend TextureLoader;
	virtual void add(SDL_Surface *img, const std::vector<TextureEntry> &entries) = 0;
	virtual void clear() = 0;
};

#endif //TEXTURE_LOADER_HPP_
