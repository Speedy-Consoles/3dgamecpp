#ifndef TEXTURE_MANAGER_HPP_
#define TEXTURE_MANAGER_HPP_

#include "texture_loader.hpp"

#include <unordered_map>
#include <vector>
#include <list>

#include <GL/glew.h>
#include <GL/gl.h>

#include "shared/engine/std_types.hpp"
#include "shared/block_utils.hpp"
#include "client/config.hpp"

struct SDL_Surface;

class TextureLoader;
class Client;

class TextureManager : public AbstractTextureManager {
public:
	struct Entry {
		GLuint tex;
		TextureType type;
		int index;
	};

	TextureManager(Client *);
	~TextureManager();

	// not copyable or movable
	TextureManager(const TextureManager &) = delete;
	TextureManager(TextureManager &&) = delete;
	TextureManager &operator = (const TextureManager &) = delete;
	TextureManager &operator = (TextureManager &&) = delete;

	void setConfig(const GraphicsConf &, const GraphicsConf &);
	
	Entry get(uint block, uint8 dir = DIR_EAST) const;
	Entry get(uint block, vec3i64 bc, uint8 dir) const;

	static void getVertices(const Entry &entry, vec2f out[4]);

protected:
	void add(SDL_Surface *img, const std::vector<TextureLoadEntry> &entries) override;
	void clear() override;

private:
	std::unordered_map<int, Entry> textures;
	std::list<GLuint> loadedTextures;

	GLuint loadTexture(int block, uint8 dir, SDL_Surface *, TextureType type);
};

#endif // TEXTURE_MANAGER_HPP_
