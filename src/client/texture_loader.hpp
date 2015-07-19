#ifndef TEXTURE_LOADER_HPP_
#define TEXTURE_LOADER_HPP_

#include <SDL2/SDL.h>

#include <string>
#include <vector>

class TextureLoader;
class BlockManager;

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
public:
	int load(const char *, const BlockManager *bm);

protected:
	friend TextureLoader;
	virtual void add(SDL_Surface *img, const std::vector<TextureEntry> &entries) = 0;
};

#endif //TEXTURE_LOADER_HPP_
