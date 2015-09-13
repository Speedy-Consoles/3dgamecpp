#ifndef SAVES_HPP_
#define SAVES_HPP_

#include <memory>
#include <string>

#include "shared/engine/vmath.hpp"

class WorldGenerator;
class ChunkArchive;

class Save {
public:
	Save(std::string);
	~Save();
	
	std::string getId() const { return _id; }
	std::string getPath() const { return _path; }
	std::string getName() const { return _name; }
	uint64 getSeed() const { return _seed; }
	vec3i64 getSpawn() const { return _spawn; }
	bool isGood() const { return _good; }

	void initialize(std::string name, uint64 seed);
	void store();

	std::unique_ptr<WorldGenerator> getWorldGenerator() const;
	std::unique_ptr<ChunkArchive> getChunkArchive() const;

private:
	std::string _id;
	std::string _path;
	std::string _name;
	uint64 _seed = 0;
	vec3i64 _spawn;
	bool _good = true;
};

#endif // SAVES_HPP_
