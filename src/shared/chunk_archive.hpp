#ifndef CHUNK_ARCHIVE_HPP_
#define CHUNK_ARCHIVE_HPP_

#include <fstream>
#include <string>
#include <unordered_map>

#include "engine/vmath.hpp"
#include "engine/macros.hpp"
#include "engine/time.hpp"

#include "game/chunk.hpp"

class ArchiveFile;

class ChunkArchive {
public:
	~ChunkArchive();
	ChunkArchive(const char *);

	ChunkArchive() = delete;
	ChunkArchive(const ChunkArchive &) = delete;
	ChunkArchive(ChunkArchive &&) = delete;

	ChunkArchive &operator = (const ChunkArchive &) = delete;
	ChunkArchive &operator = (ChunkArchive &&) = delete;

	bool hasChunk(vec3i64);

	bool loadChunk(Chunk *);
	void storeChunk(const Chunk &);

	/// clears all file handles that were not used recently
	/// clean(seconds(1)) closes all handles that were not accessed for more than one second
	void clean(Time t = 0);

private:
	ArchiveFile *getArchiveFile(vec3i64);

	std::string _path;
	std::unordered_map<vec3i64, ArchiveFile *, size_t(*)(vec3i64)> _file_map;
};

#endif // CHUNK_ARCHIVE_HPP_
