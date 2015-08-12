#ifndef TEXTURE_MANAGER_HPP_
#define TEXTURE_MANAGER_HPP_

#include <vector>

#include "shared/engine/vmath.hpp"

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

	static uint8 scrambleBlockCoordinate(vec3i64 bc);
	static void getEdgeCoordinate(vec3i64 bc, uint8 dir, 
			vec3i64 &vl, vec3i64 &vr, vec3i64 &vb, vec3i64 &vt);
	static void getTextureCoords(int index, TextureType type, vec2f out[4]);
	static int getMultitileIndex(TextureType type, vec3i64 bc, uint8 dir);

protected:
	friend TextureLoader;
	virtual void add(SDL_Surface *img, const std::vector<TextureLoadEntry> &entries) = 0;
	virtual void clear() = 0;
};

#endif // TEXTURE_MANAGER_HPP_
