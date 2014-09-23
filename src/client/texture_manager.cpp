/*
 * texture_manager.cpp
 *
 *  Created on: 19.09.2014
 *      Author: lars
 */

#include "texture_manager.hpp"

#include <SDL2/SDL_image.h>

#include "io/logging.hpp"

static const auto TEX2D = GL_TEXTURE_2D;

TextureManager::TextureManager(const GraphicsConf &conf) :
	conf(conf)
{
	// nothing
}

TextureManager::~TextureManager() {
	for (auto iter : loadedTextures) {
		GLuint tex = iter;
		glDeleteTextures(1, &tex);
	}

	for (auto iter : dictionary) {
		delete[] iter.blocks;
	}
}

/*
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
*/

static void setLoadingOptions(GraphicsConf &conf) {
	bool mipmapping = conf.tex_mipmapping > 0;
	GLenum mag_filter, min_filter;

	switch (conf.tex_filtering) {
	case TexFiltering::LINEAR:
		mag_filter = GL_LINEAR;
		min_filter = mipmapping ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
		break;
	case TexFiltering::NEAREST:
		mag_filter = GL_NEAREST;
		min_filter = mipmapping ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST;
		break;
	default:
		LOG(ERROR, "Reached unreachable code");
		mag_filter = 0;
		min_filter = 0;
		break;
	}
	glTexParameteri(TEX2D, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(TEX2D, GL_TEXTURE_MIN_FILTER, min_filter);
	if (mipmapping) {
		glTexParameteri(TEX2D, GL_TEXTURE_MAX_LEVEL, conf.tex_mipmapping);
		glTexParameteri(TEX2D, GL_GENERATE_MIPMAP, GL_TRUE);
	}
}

void TextureManager::loadTextures(const char *filename, int xTiles, int yTiles, uint *blocks) {
	SDL_Surface *img = IMG_Load(filename);
	if (!img) {
		LOG(ERROR, "File '" << filename << "' could not be loaded");
		return;
	}
	int tileW = img->w / xTiles;
	int tileH = img->h / yTiles;
	SDL_Surface *tmp = SDL_CreateRGBSurface(
			0, tileW, tileH, 32,
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if (!tmp) {
		LOG(ERROR, "Temporary SDL_Surface could not be created");
		return;
	}

	size_t n = xTiles * yTiles;
	GLuint *texs = new GLuint[n];
	glGenTextures(n, texs);

	size_t loaded = 0;
	auto blocks_ptr = blocks;
	float texW = 1.0 / xTiles;
	float texH = 1.0 / yTiles;
	for (int j = 0; j < yTiles; ++j) {
		for (int i = 0; i < xTiles; ++i) {
			uint block = *blocks_ptr++;
			if (block == 0)
				continue;
			GLuint tex = *texs++;

			glBindTexture(TEX2D, tex);
			setLoadingOptions(conf);
			SDL_Rect rect{i * tileW, j * tileH, tileW, tileH};
			int ret_code = SDL_BlitSurface(img, &rect, tmp, nullptr);
			if (ret_code)
				LOG(ERROR, "Blit unsuccessful: " << SDL_GetError());
			glTexImage2D(TEX2D, 0, 4, tileW, tileH, 0,
					GL_RGBA, GL_UNSIGNED_BYTE, tmp->pixels);

			Entry entry{block, tex, i * texW, j * texH, texW, texH};
			textures.insert({block, entry});
			loadedTextures.push_back(tex);
			++loaded;
		}
	}

	if (loaded < n)
		glDeleteTextures(n - loaded, texs);

	SDL_FreeSurface(tmp);
	SDL_FreeSurface(img);

	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		LOG(ERROR, "Loading '" << filename << "': " << gluErrorString(e));
	}

	uint *blocks_copy = new uint[n];
	memcpy(blocks_copy, blocks, n * sizeof (uint));
	dictionary.push_back(DictEntry{filename, xTiles, yTiles, blocks_copy});

	LOG(DEBUG, "Loaded " << loaded << " textures from '" << filename << "'");
}

GLuint TextureManager::loadTexture(const char *filename, uint block) {
	SDL_Surface *img = IMG_Load(filename);
	if (!img) {
		LOG(ERROR, "file '" << filename << "' could not be loaded");
		return 0;
	}
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(TEX2D, tex);
	setLoadingOptions(conf);
	glTexImage2D(TEX2D, 0, 4, img->w, img->h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);
	SDL_FreeSurface(img);

	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		LOG(ERROR, "Loading '" << filename << "': " << gluErrorString(e));
	}

	Entry entry{block, tex, 0, 0, 1, 1};
	textures.insert({block, entry});
	loadedTextures.push_back(tex);

	uint *blocks_copy = new uint[1];
	blocks_copy[0] = block;
	dictionary.push_back(DictEntry{filename, 1, 1, blocks_copy});
	return tex;
}

void TextureManager::setConfig(const GraphicsConf &c) {
	GraphicsConf old_conf = conf;
	conf = c;

	if (conf.tex_mipmapping != old_conf.tex_mipmapping ||
			conf.tex_filtering != old_conf.tex_filtering)
	{
		for (auto iter : loadedTextures) {
			GLuint tex = iter;
			glBindTexture(TEX2D, tex);
			setLoadingOptions(conf);
		}
	}
}

void TextureManager::reloadAll() {
	for (auto iter : loadedTextures) {
		GLuint tex = iter;
		glDeleteTextures(1, &tex);
	}
	textures.clear();
	loadedTextures.clear();

	std::vector<DictEntry> old_dictionary;
	old_dictionary.swap(dictionary);

	for (auto iter : old_dictionary) {
		loadTextures(iter.path.c_str(), iter.xTiles, iter.yTiles, iter.blocks);
		delete[] iter.blocks;
	}
}

void TextureManager::bind(uint block) {
	if (block == 0) {
		glBindTexture(TEX2D, 0);
	} else if (block == lastBound.block) {
		// nothing
	} else {
		auto iter = textures.find(block);
		if (iter == textures.end()) {
			LOG(TRACE, "texture " << block << " not found");
			glBindTexture(TEX2D, 0);
		} else {
			Entry entry = iter->second;
			glBindTexture(TEX2D, entry.tex);

			GLenum e = glGetError();
			if (e != GL_NO_ERROR)
				LOG(ERROR, "Binding texture " << entry.tex << ": " << gluErrorString(e));
		}
	}
}

