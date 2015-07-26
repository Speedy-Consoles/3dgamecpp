/*
 * texture_manager.hpp
 *
 *  Created on: 19.09.2014
 *      Author: lars
 */

#ifndef TEXTURE_MANAGER_HPP_
#define TEXTURE_MANAGER_HPP_

#include "texture_loader.hpp"

#include <GL/glew.h>
#include <GL/gl.h>
#include <unordered_map>
#include <vector>
#include <list>

#include "engine/std_types.hpp"
#include "config.hpp"

struct SDL_Surface;

class TextureLoader;
class Client;

class TextureManager : public AbstractTextureManager {
public:
	TextureManager(Client *);
	~TextureManager();

	// not copyable or movable
	TextureManager(const TextureManager &) = delete;
	TextureManager(TextureManager &&) = delete;
	TextureManager &operator = (const TextureManager &) = delete;
	TextureManager &operator = (TextureManager &&) = delete;

	void setConfig(const GraphicsConf &, const GraphicsConf &);

	bool isWangTileBound() const;

	bool bind(uint block);
	GLuint getTexture();
	void getTextureVertices(vec2f out[4]) const;
	void getTextureVertices(vec3i64 bc, uint8 dir, vec2f out[4]) const;

protected:
	void add(SDL_Surface *img, const std::vector<TextureEntry> &entries) override;
	void clear() override;

private:
	struct Entry {
		uint block;
		GLuint tex;
		TextureType type;
		float x, y, w, h;
	};
	std::unordered_map<uint, Entry> textures;
	Entry lastBound = Entry{0, 0, TextureType::SINGLE_TEXTURE, 0, 0, 1, 1};

	std::list<GLuint> loadedTextures;

	void bind(uint block, GLenum primitive, bool endPrimitive);
	GLuint loadTexture(uint block, SDL_Surface *, TextureType type);
};

#endif // TEXTURE_MANAGER_HPP_
