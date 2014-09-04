/*
 * archive.hpp
 *
 *  Created on: Sep 3, 2014
 *      Author: lars
 */

#ifndef ARCHIVE_HPP
#define ARCHIVE_HPP

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>

#include "std_types.hpp"
#include "vmath.hpp"
#include "util.hpp"
#include "chunk.hpp"

class ArchiveFile {
public:

	static const char MAGIC[4];

	enum Endianess {
		UNKNOWN = -1,
		NATIVE,
		FLIPPED
	};

	~ArchiveFile();
	ArchiveFile(const char *, size_t size = 16);

	ArchiveFile() = delete;
	ArchiveFile(const ArchiveFile &) = delete;
	ArchiveFile(ArchiveFile &&) = delete;

	ArchiveFile &operator = (const ArchiveFile &) = delete;
	ArchiveFile &operator = (ArchiveFile &&) = delete;

	void initialize();

	Chunk *loadChunk(vec3i64);
	void storeChunk(const Chunk &);

private:
	std::fstream _file;

	Endianess _endianess;
	int32 _version;
	size_t _region_size;
	size_t _directory_offset;
};

class ChunkArchive {
public:
	~ChunkArchive();
	ChunkArchive(const char *);

	ChunkArchive() = delete;
	ChunkArchive(const ChunkArchive &) = delete;
	ChunkArchive(ChunkArchive &&) = delete;

	ChunkArchive &operator = (const ChunkArchive &) = delete;
	ChunkArchive &operator = (ChunkArchive &&) = delete;

	Chunk *loadChunk(vec3i64);
	void storeChunk(const Chunk &);

	void clearHandles();

private:
	ArchiveFile *getArchiveFile(vec3i64);

	std::string _path;
	std::unordered_map<vec3i64, ArchiveFile *, size_t(*)(vec3i64)> _file_map;
};

#endif // ARCHIVE_HPP
