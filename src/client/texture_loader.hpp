#ifndef TEXTURE_LOADER_HPP_
#define TEXTURE_LOADER_HPP_

#include <SDL2/SDL.h>

#include <string>
#include <vector>
#include <memory>

#include "engine/std_types.hpp"

class TextureLoader;
class Client;

enum class TextureType {
	SINGLE_TEXTURE,
	WANG_TILES,
	MULTI_x4,
};

struct TextureLoadEntry {
	int id;
	TextureType type;
	uint8 dir_mask;
	int x, y, w, h;
};

// this makes sure that smart pointers of SDL_Surfaces delete the object properly
namespace std {
	template<>
	void default_delete<SDL_Surface>::operator()(SDL_Surface* s) const;
}

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
	virtual void add(SDL_Surface *img, const std::vector<TextureLoadEntry> &entries) = 0;
	virtual void clear() = 0;
};

#endif //TEXTURE_LOADER_HPP_
