/*
 * texture_manager.hpp
 *
 *  Created on: 19.09.2014
 *      Author: lars
 */

#ifndef TEXTURE_MANAGER_HPP_
#define TEXTURE_MANAGER_HPP_

#include <GL/glew.h>
#include <GL/gl.h>
#include <unordered_map>

#include "std_types.hpp"

class TextureManager {
public:
	TextureManager();
	~TextureManager();

	// not copyable or movable
	TextureManager(const TextureManager &) = delete;
	TextureManager(TextureManager &&) = delete;
	TextureManager &operator = (const TextureManager &) = delete;
	TextureManager &operator = (TextureManager &&) = delete;

	GLuint loadAtlas(const char *filename, int xTiles, int yTiles, uint *blocks);
	void loadTextures(const char *filename, int xTiles, int yTiles, uint *blocks);
	GLuint loadTexture(const char *filename, uint block);

	void bind(uint block);

private:
	struct Entry {
		uint block;
		GLuint tex;
		float x, y, w, h;
	};

	std::unordered_map<uint, Entry> textures;

	Entry lastBound = Entry{0, 0, 0, 0, 0, 0};
};

#endif // TEXTURE_MANAGER_HPP_
