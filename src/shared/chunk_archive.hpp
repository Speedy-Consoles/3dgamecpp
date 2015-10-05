#ifndef CHUNK_ARCHIVE_HPP_
#define CHUNK_ARCHIVE_HPP_

#include <fstream>
#include <string>
#include <unordered_map>

#include "engine/vmath.hpp"
#include "engine/macros.hpp"
#include "engine/time.hpp"
#include "engine/rwlock.hpp"

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

	/** Check whether a chunk was stored in the past

		If a chunk exists and revision is not nullptr, the current revision of the chunk is written
		to the address pointed to by revision.

		Calling any number of instances of this function and up to one instance of either
		loadChunk or storeChunk concurrently is safe.
	*/
	bool hasChunk(vec3i64, uint32 *revision = nullptr);

	/** Load and store chunks

		Calling any number of instances of hasChunk concurrently to these functions is safe.  
		However, calling more than one instance of either loadChunk or storeChunk concurrently
		is NOT safe.  Only one thread should ever make calls to these functions.
	*/
	bool loadChunk(Chunk *);
	void storeChunk(const Chunk &);

	/** Closes all file handles that were not used recently

		E.g. clean(seconds(1)) closes all handles that were not accessed for more than one second
		and clean() closes all file handles.
	*/
	void clean(Time t = 0);

private:
	ArchiveFile *unsafe_getArchiveFile(vec3i64);
	void unsafe_addArchiveFile(vec3i64);
	void unsafe_clean(Time t = 0);

	std::string _path;
	std::unordered_map<vec3i64, ArchiveFile *, size_t(*)(vec3i64)> _file_map;
	ReadWriteLock _file_map_lock;
};

#endif // CHUNK_ARCHIVE_HPP_
