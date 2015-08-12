#include "texture_manager.hpp"

#include "shared/engine/logging.hpp"
#include "client/gfx/texture_loader.hpp"
#include "client/client.hpp"

static logging::Logger logger("gfx");

int TextureManager::load(const char *path) {
	files.push_back(path);
	auto *bm = _client->getBlockManager();
	auto loader = std::unique_ptr<TextureLoader>(new TextureLoader(path, bm, this));
	try {
		loader->load();
	} catch (TextureLoader::ParsingError &e) {
		LOG_ERROR(logger) << path << ":" << (e.row + 1) << ":" << (e.col + 1) << ": " << e.error;
		return 1;
	}
	return 0;
}

int TextureManager::reloadAll() {
	clear();
	int result = 0;
	for (std::string &file : files) {
		result |= load(file.c_str());
	}
	return result;
}