#include "saves.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>
#include <boost/filesystem.hpp>

#include "shared/engine/logging.hpp"
#include "shared/game/world_generator.hpp"
#include "shared/chunk_archive.hpp"

using namespace std;
using namespace boost;
using namespace boost::property_tree;

static logging::Logger logger("io");

Save::Save(std::string id) :
	_id(id), _path(string("./saves/") + id + "/")
{
	boost::filesystem::path path(_path);
	if (!boost::filesystem::exists(path)) {
		LOG_WARNING(logger) << "World '" << id << "' does not exist";
		_good = false;
		return;
	}

	ptree pt;

	string filename = string(_path) + "world.txt";
	try {
		read_info(filename, pt);
	} catch (...) {
		LOG_ERROR(logger) << "'" << filename << "' could not be opened";
		_good = false;
	}

	try {
		_name = pt.get<string>("world.name");
		_seed = pt.get<uint64>("world.seed");
	} catch (...) {
		LOG_ERROR(logger) << "'" << filename << "' is invalid";
		_good = false;
	}

	bool needs_new_spawn = false;
	if (!pt.get_child_optional("world.spawn")) {
		needs_new_spawn = true;
	} else {
		try {
			const int64 x = pt.get<int64>("world.spawn.x");
			const int64 y = pt.get<int64>("world.spawn.y");
			const int64 z = pt.get<int64>("world.spawn.z");
			_spawn = vec3i64(x, y, z);
		} catch (...) {
			needs_new_spawn = true;
		}
	}

	if (needs_new_spawn) {
		LOG_WARNING(logger) << "'" << filename << "' did not contain a valid spawn";
		auto world_gen = getWorldGenerator();
		_spawn = world_gen->getSpawnLocation();
		LOG_INFO(logger) << "(" << _spawn << ") is now the new spawn";
		store();
	}
}

Save::~Save() {
	// nothing
}

void Save::initialize(std::string name, uint64 seed) {
	_name = name;
	_seed = seed;
	auto world_gen = getWorldGenerator();
	_spawn = world_gen->getSpawnLocation();
	store();
}

void Save::store() {
	ptree pt;
	
	pt.put("world.name", _name);
	pt.put("world.seed", _seed);
	pt.put("world.spawn.x", _spawn[0]);
	pt.put("world.spawn.y", _spawn[1]);
	pt.put("world.spawn.z", _spawn[2]);
	
	string filename = string(_path) + "world.txt";
	write_info(filename, pt);
}

unique_ptr<WorldGenerator> Save::getWorldGenerator() const {
	WorldGenerator *p_world_gen = new WorldGenerator(_seed, WorldParams());
	return unique_ptr<WorldGenerator>(p_world_gen);
}

unique_ptr<ChunkArchive> Save::getChunkArchive() const {
	string filename = string(_path) + "region/";
	ChunkArchive *p_chunk_archive = new ChunkArchive(filename.c_str());
	return unique_ptr<ChunkArchive>(p_chunk_archive);
}
