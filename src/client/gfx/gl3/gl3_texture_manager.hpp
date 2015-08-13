#ifndef GL3_TEXTURE_MANAGER_HPP_
#define GL3_TEXTURE_MANAGER_HPP_

#include "client/gfx/texture_loader.hpp"
#include "client/gfx/texture_manager.hpp"

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

class GL3TextureManager : public TextureManager {
public:
	struct Entry {
		GLuint tex;
		GLuint layer;
		TextureType type;
		int index;
	};

	GL3TextureManager(Client *);
	~GL3TextureManager();

	void setConfig(const GraphicsConf &, const GraphicsConf &);
	
	Entry get(uint block, uint8 dir = DIR_EAST) const;
	Entry get(uint block, vec3i64 bc, uint8 dir) const;

protected:
	void add(SDL_Surface *img, const std::vector<TextureLoadEntry> &entries) override;
	void clear() override;

private:
	std::unordered_map<int, Entry> textures;
	std::list<GLuint> loadedTextures;

	GLuint blockTextures = 0;
};

#endif // GL3_TEXTURE_MANAGER_HPP_
