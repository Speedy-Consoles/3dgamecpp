#include "gl2_texture_manager.hpp"

#include <SDL2/SDL_image.h>

#include "shared/engine/logging.hpp"
#include "shared/block_utils.hpp"
#include "client/client.hpp"


static logging::Logger logger("gfx");

static const auto TEX2D = GL_TEXTURE_2D;

GL2TextureManager::GL2TextureManager(Client *client) :
	TextureManager(client)
{
	// nothing
}

GL2TextureManager::~GL2TextureManager() {
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

void GL2TextureManager::setConfig(const GraphicsConf &conf, const GraphicsConf &old) {
	if (conf.tex_mipmapping != old.tex_mipmapping || conf.tex_filtering != old.tex_filtering) {
		for (auto iter : loadedTextures) {
			GLuint tex = iter;
			GL(BindTexture(TEX2D, tex));
			setLoadingOptions(conf);
		}
	}
}

auto GL2TextureManager::get(uint block, uint8 dir) const -> Entry {
	int key = (int) block;
	key <<= 3;
	key |= dir;
	auto iter = textures.find(key);
	if (iter == textures.end()) {
		iter = textures.find(-1);
	}
	if (iter == textures.end()) {
		static Entry invalid = Entry{0, TextureType::SINGLE_TEXTURE, -1};
		return invalid;
	}
	Entry entry = iter->second;
	return entry;
}

auto GL2TextureManager::get(uint block, vec3i64 bc, uint8 dir) const -> Entry {
	Entry entry = get(block, dir);
	entry.index = getMultitileIndex(entry.type, bc, dir);
	return entry;
}

void GL2TextureManager::add(SDL_Surface *img, const std::vector<TextureLoadEntry> &entries) {
	std::unique_ptr<SDL_Surface> tmp;

	for (const auto &entry : entries) {
		int width = entry.w > 0 ? entry.w : img->w;
		int height = entry.h > 0 ? entry.h : img->h;
		if (!tmp || tmp->w != width || tmp->h != height) {
			tmp = std::unique_ptr<SDL_Surface>(SDL_CreateRGBSurface(
					0, width, height, 32,
					0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000));
			if (!tmp) {
				LOG_ERROR(logger) << "Temporary SDL_Surface could not be created";
				return;
			}
		}

		SDL_Rect rect{entry.x, entry.y, width, height};
		int ret_code = SDL_BlitSurface(img, &rect, tmp.get(), nullptr);
		if (ret_code)
			LOG_ERROR(logger) << "Blit unsuccessful: " << SDL_GetError();

		loadTexture(entry.id, entry.dir_mask, tmp.get(), entry.type);
	}
}

void GL2TextureManager::clear() {
	for (auto iter : loadedTextures) {
		GLuint tex = iter;
		GL(DeleteTextures(1, &tex));
	}
	textures.clear();
	loadedTextures.clear();
}

GLuint GL2TextureManager::loadTexture(int block, uint8 dir_mask, SDL_Surface *img, TextureType type) {
	GLuint tex;
	GL(GenTextures(1, &tex));
	GL(BindTexture(TEX2D, tex));
	setLoadingOptions(_client->getConf());
	GL(TexImage2D(TEX2D, 0, 4, img->w, img->h, 0,
			GL_RGBA, GL_UNSIGNED_BYTE, img->pixels));

	Entry entry = Entry{tex, type, -1};
	if (block < 0) {
		textures[(int) block] = entry;
	} else {
		for (uint dir = 0; dir < 6; ++dir) {
			if (dir_mask & (1 << dir)) {
				int key = (int) ((block << 3) | (int) dir);
				textures[key] = entry;
			}
		}
	}
	loadedTextures.push_back(tex);

	return tex;
}
