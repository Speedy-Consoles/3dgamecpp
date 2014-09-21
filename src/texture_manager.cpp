/*
 * texture_manager.cpp
 *
 *  Created on: 19.09.2014
 *      Author: lars
 */

#include "texture_manager.hpp"

#include <SDL2/SDL_image.h>

#include "logging.hpp"

static const auto TEX2D = GL_TEXTURE_2D;

TextureManager::TextureManager() {
	// nothing
}

TextureManager::~TextureManager() {
	for (auto iter : textures) {
		glDeleteTextures(1, &iter.second.tex);
	}
}

GLuint TextureManager::loadAtlas(const char *filename, int xTiles, int yTiles, uint *blocks) {
	SDL_Surface *img = IMG_Load(filename);
	if (!img)
		LOG(ERROR, "file '" << filename << "' could not be loaded");

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(TEX2D, tex);
	glTexParameteri(TEX2D, GL_TEXTURE_MAX_LOD, 6);
	glTexParameteri(TEX2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(TEX2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(TEX2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(
			TEX2D, 0, 4, img->w, img->h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, img->pixels
	);
	SDL_FreeSurface(img);

	float w = 1.0 / xTiles;
	float h = 1.0 / yTiles;
	for (int y = 0; y < yTiles; ++y) {
		for (int x = 0; x < xTiles; ++x) {
			uint block = *blocks++;
			Entry entry{block, tex, x * w, y * h, w, h};
			textures.insert({block, entry});
		}
	}

	GLenum e = glGetError();
	if (e != GL_NO_ERROR)
		LOG(ERROR, "Loading '" << filename << "': " << gluErrorString(e));

	return tex;
}

void TextureManager::loadTextures(const char *filename, int xTiles, int yTiles, uint *blocks) {
	SDL_Surface *img = IMG_Load(filename);
	if (!img)
		LOG(ERROR, "file '" << filename << "' could not be loaded");
	int tileW = img->w / xTiles;
	int tileH = img->h / yTiles;
	SDL_Surface *tmp = SDL_CreateRGBSurface(
			0, tileW, tileH, 32,
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

	size_t n = xTiles * yTiles;
	GLuint *texs = new GLuint[n];
	glGenTextures(n, texs);

	size_t loaded = 0;
	float texW = 1.0 / xTiles;
	float texH = 1.0 / yTiles;
	for (int j = 0; j < yTiles; ++j) {
		for (int i = 0; i < xTiles; ++i) {
			uint block = *blocks++;
			if (block == 0)
				continue;
			GLuint tex = *texs++;

			glBindTexture(TEX2D, tex);
			glTexParameteri(TEX2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(TEX2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(TEX2D, GL_GENERATE_MIPMAP, GL_TRUE);
			SDL_Rect rect{i * tileW, j * tileH, tileW, tileH};
			int ret_code = SDL_BlitSurface(img, &rect, tmp, nullptr);
			if (ret_code)
				LOG(ERROR, "Blit unsuccessful: " << SDL_GetError());
			glTexImage2D(TEX2D, 0, 4, tileW, tileH, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, tmp->pixels);

			Entry entry{block, tex, i * texW, j * texH, texW, texH};
			textures.insert({block, entry});
			++loaded;
		}
	}

	if (loaded < n)
		glDeleteTextures(n - loaded, texs);

	LOG(DEBUG, "Loaded " << loaded << " textures from '" << filename << "'");

	GLenum e = glGetError();
	if (e != GL_NO_ERROR)
		LOG(ERROR, "Loading '" << filename << "': " << gluErrorString(e));

	SDL_FreeSurface(tmp);
	SDL_FreeSurface(img);
}

GLuint TextureManager::loadTexture(const char *filename, uint block) {
	SDL_Surface *img = IMG_Load(filename);
	if (!img)
		LOG(ERROR, "file '" << filename << "' could not be loaded");
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(TEX2D, tex);
	glTexParameteri(TEX2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(TEX2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(TEX2D, GL_GENERATE_MIPMAP, GL_TRUE);
	glTexImage2D(TEX2D, 0, 4, img->w, img->h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);
	SDL_FreeSurface(img);

	GLenum e = glGetError();
	if (e != GL_NO_ERROR)
		LOG(ERROR, "Loading '" << filename << "': " << gluErrorString(e));

	Entry entry{block, tex, 0, 0, 1, 1};
	textures.insert({block, entry});
	return entry.tex;
}

void TextureManager::bind(uint block) {
	if (block == lastBound.block)
		return;
	auto iter = textures.find(block);
	if (iter == textures.end()) {
		LOG(ERROR, "texture " << block << " not found");
		return;
	}
	Entry entry = iter->second;
	glBindTexture(TEX2D, entry.tex);

	GLenum e = glGetError();
	if (e != GL_NO_ERROR)
		LOG(ERROR, "Binding texture " << entry.tex << ": " << gluErrorString(e));
}
