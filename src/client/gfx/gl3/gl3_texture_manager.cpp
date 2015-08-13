#include "gl3_texture_manager.hpp"

#include <SDL2/SDL_image.h>

#include "shared/engine/logging.hpp"
#include "shared/block_utils.hpp"
#include "client/client.hpp"

static logging::Logger logger("gfx");

static const auto TEX2D = GL_TEXTURE_2D;

GL3TextureManager::GL3TextureManager(Client *client) :
	TextureManager(client)
{
	// nothing
}

GL3TextureManager::~GL3TextureManager() {
	clear();
}

static void setLoadingOptions(const GraphicsConf &conf) {
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
		LOG_ERROR(logger) << "Reached unreachable code";
		mag_filter = 0;
		min_filter = 0;
		break;
	}
	GL(TexParameteri(TEX2D, GL_TEXTURE_MAG_FILTER, mag_filter));
	GL(TexParameteri(TEX2D, GL_TEXTURE_MIN_FILTER, min_filter));
	if (mipmapping) {
		GL(TexParameteri(TEX2D, GL_TEXTURE_MAX_LEVEL, conf.tex_mipmapping));
		GL(TexParameteri(TEX2D, GL_GENERATE_MIPMAP, GL_TRUE));
	}
}

void GL3TextureManager::setConfig(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.tex_mipmapping != old.tex_mipmapping || conf.tex_filtering != old.tex_filtering) {
		for (auto iter : loadedTextures) {
			GLuint tex = iter;
			GL(BindTexture(TEX2D, tex));
			setLoadingOptions(conf);
		}
	}
}

auto GL3TextureManager::get(uint block, uint8 dir) const -> Entry {
	int key = (int) block;
	key <<= 3;
	key |= dir;
	auto iter = textures.find(key);
	if (iter == textures.end()) {
		iter = textures.find(-1);
	}
	if (iter == textures.end()) {
		static Entry invalid = Entry{0, 0, TextureType::SINGLE_TEXTURE, -1};
		return invalid;
	}
	Entry entry = iter->second;
	return entry;
}

auto GL3TextureManager::get(uint block, vec3i64 bc, uint8 dir) const -> Entry {
	Entry entry = get(block, dir);
	entry.index = getMultitileIndex(entry.type, bc, dir);
	return entry;
}

void GL3TextureManager::add(SDL_Surface *img, const std::vector<TextureLoadEntry> &entries) {
	if (blockTextures)
		return;

	int xTiles = 16;
	int yTiles = 16;
	GLsizei layerCount = entries.size();
	GLsizei mipLevelCount = 7;

	GLint maxLayerCount;
	GL(GetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxLayerCount));
	if (maxLayerCount < layerCount) {
		LOG_ERROR(logger) << "Not enough levels available for texture";
		return;
	}

	int tileW = img->w / xTiles;
	int tileH = img->h / yTiles;
	SDL_Surface *tmp = SDL_CreateRGBSurface(
			0, tileW, tileH, 32,
			0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
	if (!tmp) {
		LOG_ERROR(logger) << "Temporary SDL_Surface could not be created";
		return;
	}

	GL(GenTextures(1, &blockTextures));
	GL(BindTexture(GL_TEXTURE_2D_ARRAY, blockTextures));
	GL(TexStorage3D(GL_TEXTURE_2D_ARRAY, mipLevelCount, GL_RGBA8, tileW, tileH, layerCount));

	GLuint layerIndex = 0;
	for (const auto &loadEntry : entries) {
		SDL_Rect rect{loadEntry.x, loadEntry.y, tileW, tileH};
		int ret_code = SDL_BlitSurface(img, &rect, tmp, nullptr);
		if (ret_code)
			LOG_ERROR(logger) << "Blit unsuccessful: " << SDL_GetError();
		GL(TexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, layerIndex, tileW, tileH, 1, GL_RGBA, GL_UNSIGNED_BYTE, tmp->pixels));
	
		Entry entry = Entry{blockTextures, layerIndex, TextureType::SINGLE_TEXTURE, -1};
		if (loadEntry.id < 0) {
			textures[loadEntry.id] = entry;
		} else {
			for (uint dir = 0; dir < 6; ++dir) {
				if (loadEntry.dir_mask & (1 << dir)) {
					int key = (int) ((loadEntry.id << 3) | (int) dir);
					textures[key] = entry;
				}
			}
		}
		++layerIndex;
	}

	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0));
	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, mipLevelCount - 1));
	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_GENERATE_MIPMAP, GL_TRUE));

	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR));
	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT));
	GL(TexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT));
	GL(GenerateMipmap(GL_TEXTURE_2D_ARRAY));

	SDL_FreeSurface(tmp);
}

void GL3TextureManager::clear() {
	for (auto iter : loadedTextures) {
		GLuint tex = iter;
		GL(DeleteTextures(1, &tex));
	}
	textures.clear();
	loadedTextures.clear();
}
