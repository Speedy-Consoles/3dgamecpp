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
}

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

void TextureManager::loadTextures(uint *blocks, const char *filename, int xTiles, int yTiles) {
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

	size_t loaded = 0;
	auto blocks_ptr = blocks;
	for (int j = 0; j < yTiles; ++j) {
		for (int i = 0; i < xTiles; ++i) {
			uint block = *blocks_ptr++;
			if (block == 0)
				continue;
			SDL_Rect rect{i * tileW, j * tileH, tileW, tileH};
			int ret_code = SDL_BlitSurface(img, &rect, tmp, nullptr);
			if (ret_code)
				LOG(ERROR, "Blit unsuccessful: " << SDL_GetError());
			loadTexture(block, tmp, SINGLE_TEXTURE);
			++loaded;
		}
	}

	SDL_FreeSurface(tmp);
	SDL_FreeSurface(img);

	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		LOG(ERROR, "Loading '" << filename << "': " << gluErrorString(e));
	}

	LOG(DEBUG, "Loaded " << loaded << " textures from '" << filename << "'");
}

GLuint TextureManager::loadTexture(uint block, const char *filename, TextureType type) {
	SDL_Surface *img = IMG_Load(filename);
	if (!img) {
		LOG(ERROR, "File '" << filename << "' could not be opened");
		return 0;
	}
	GLuint tex = loadTexture(block, img, type);
	SDL_FreeSurface(img);
	if (!tex) {
		LOG(ERROR, "File '" << filename << "' could not be opened");
		return 0;
	}
	return tex;
}

GLuint TextureManager::loadTexture(uint block, SDL_Surface *img, TextureType type) {
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(TEX2D, tex);
	setLoadingOptions(conf);
	glTexImage2D(TEX2D, 0, 4, img->w, img->h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, img->pixels);

	GLenum e = glGetError();
	if (e != GL_NO_ERROR) {
		LOG(ERROR, "Could not load texture: " << gluErrorString(e));
		return 0;
	}

	Entry entry{block, tex, type, 0, 0, 1, 1};
	textures.insert({block, entry});
	loadedTextures.push_back(tex);

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

void TextureManager::bind(uint block, GLenum primitive, bool endPrimitive) {
	if (block == lastBound.block)
		return;

	// figure out which texture to bind
	Entry nextBound = lastBound;
	if (block == 0) {
		nextBound = {0, 0, SINGLE_TEXTURE, 0, 0, 1, 1};
	} else {
		auto iter = textures.find(block);
		if (iter == textures.end()) {
			LOG(TRACE, "texture " << block << " not found");
			nextBound = {0, 0, SINGLE_TEXTURE, 0, 0, 1, 1};
		} else {
			nextBound = iter->second;
		}
	}

	// bind the texture
	if (nextBound.tex != lastBound.tex) {
		if (endPrimitive) {
			glEnd();
		}
		glBindTexture(TEX2D, nextBound.tex);
		GLenum e = glGetError();
		if (e != GL_NO_ERROR) {
			LOG(ERROR, "Binding texture "
					<< nextBound.tex << ": " << gluErrorString(e));
			glBindTexture(TEX2D, 0);
		}
		if (endPrimitive) {
			glBegin(primitive);
		}
	}

	// select subtexture
	if (endPrimitive) {
		glEnd();
	}
	glMatrixMode(GL_TEXTURE);
	glPopMatrix();
	glPopMatrix();
	glPushMatrix();
	if (nextBound.type != SINGLE_TEXTURE) {
		if (nextBound.type == TEXTURE_ATLAS) {
			glScalef(nextBound.w, nextBound.h, 1);
			glTranslatef(nextBound.x, nextBound.y, 0);
		} else if (nextBound.type == WANG_TILES) {
			glScalef(0.25, 0.25, 1);
		}
	}
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	if (endPrimitive) {
		glBegin(primitive);
	}

	lastBound = nextBound;
}

bool TextureManager::isWangTileBound() const {
	return lastBound.type == WANG_TILES;
}


