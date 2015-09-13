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
		_name = pt.get<string>("world.name");
		_seed = pt.get<uint64>("world.seed");
	} catch (...) {
		LOG_ERROR(logger) << "'" << filename << "' could not be opened";
		_good = false;
	}
}

Save::~Save() {
	store();
}

void Save::initialize(std::string name, uint64 seed) {
	_name = name;
	_seed = seed;
}

void Save::store() {
	ptree pt;
	
	pt.put("world.name", _name);
	pt.put("world.seed", _seed);
	
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
