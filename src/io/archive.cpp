#include "archive.hpp"

#include <cstring>

#include <boost/filesystem.hpp>

#include "engine/math.hpp"

#include "util.hpp"

#include "engine/logging.hpp"
static logging::Logger logger("default");

using namespace std;

const char ArchiveFile::MAGIC[4] = {89, -105, 34, -33};

static const int32 ENDIANESS_BYTES = 0x01020304;
static const int32 ENDIANESS_BYTES_FLIPPED = 0x04030201;

static const uint8 ESCAPE_CHAR = (uint8) (-1);

PACKED(
struct Header {
	char magic[4];
	int32 endianess_bytes;
	int32 version;
	uint32 size;
	uint32 directory_offset;
});

PACKED(
struct DirectoryEntry {
	uint32 offset;
	uint32 size;
});

ArchiveFile::~ArchiveFile() {
	_file.close();
}

ArchiveFile::ArchiveFile(const char *filename, size_t region_size) :
	_region_size(region_size)
{
	_file.open(filename, ios_base::in | ios_base::out | ios_base::binary);
	if (_file.fail()) {
		_file.open(filename, ios_base::out);
		_file.close();
		_file.open(filename, ios_base::in | ios_base::out | ios_base::binary);
		if (!_file.good()) {
			printf("PANIC\n");
		}
	}

	bool all_ok;

	Header header;
	_file.read((char *)(&header), sizeof (Header));
	if (_file.eof()) {
		_file.clear();
		all_ok = false;
	} else {
		switch (header.endianess_bytes) {
		case ENDIANESS_BYTES:
			_endianess = NATIVE;
			break;
		case ENDIANESS_BYTES_FLIPPED:
			_endianess = FLIPPED;
			break;
		default:
			_endianess = UNKNOWN;
			break;
		}

		_version = header.version;
		size_t size = (size_t) header.size;
		_directory_offset = (size_t) header.directory_offset;

		bool is_magic_ok = memcmp(header.magic, MAGIC, 4 * sizeof (char)) == 0;
		bool is_endianess_ok = _endianess == NATIVE;
		bool is_version_ok = _version == 1;
		bool is_size_ok = size == _region_size * _region_size * _region_size;

		all_ok = is_magic_ok && is_endianess_ok && is_version_ok && is_size_ok;
	}

	if (!all_ok) {
		initialize();
	}
}

void ArchiveFile::initialize() {
	Header header;
	memcpy(header.magic, MAGIC, 4 * sizeof (char));
	header.endianess_bytes = ENDIANESS_BYTES;
	_endianess = NATIVE;
	_version = header.version = 0x0001;
	uint size = _region_size * _region_size * _region_size;
	header.size = size;
	_directory_offset = header.directory_offset = sizeof (Header);

	// write header
	_file.seekp(0);
	_file.write((char *)(&header), sizeof (Header));

	// zero the directory
	const size_t BULK_SIZE = 1024 * sizeof (DirectoryEntry);
	char *zeros = new char[BULK_SIZE];
	memset(zeros, 0, BULK_SIZE);
	for (uint i = 0; i < size / 1024; ++i) {
		_file.write(zeros, BULK_SIZE);
	}
	_file.write(zeros, (size % 1024) * sizeof (DirectoryEntry));
	_file.flush();
	delete zeros;
}

Chunk *ArchiveFile::loadChunk(vec3i64 cc) {
	Chunk *chunk = new Chunk(cc);
	if (loadChunk(cc, *chunk)) {
		return chunk;
	} else {
		delete chunk;
		return nullptr;
	}
}

bool ArchiveFile::loadChunk(Chunk &chunk) {
	return loadChunk(chunk.getCC(), chunk);
}

bool ArchiveFile::loadChunk(vec3i64 cc, Chunk &chunk) {
	// read directory entry
	size_t x = cycle(cc[0], _region_size);
	size_t y = cycle(cc[1], _region_size);
	size_t z = cycle(cc[2], _region_size);
	size_t id = x + (_region_size * (y + (_region_size * z)));
	_file.seekg(_directory_offset + id * sizeof (DirectoryEntry));
	DirectoryEntry dir_entry;
	_file.read((char *)(&dir_entry), sizeof (DirectoryEntry));

	// seek there
	if (dir_entry.offset == 0) {
		return false;
	} else {
		_file.seekg(dir_entry.offset);
	}

	// load and decode the chunk
	size_t index = 0;
	const size_t chunk_size = Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH;

	while (index < chunk_size) {
		uint8 next_block;
		_file.read((char *) &next_block, sizeof (uint8));

		if (next_block == ESCAPE_CHAR) {
			uint8 run_length, block_type;
			_file.read((char *) &run_length, sizeof (uint8));
			_file.read((char *) &block_type, sizeof (uint8));
			for (size_t i = 0; i < run_length; ++i) {
				chunk.initBlock(index++, block_type);
			}
		} else {
			chunk.initBlock(index++, next_block);
		}
	}

	if (!_file.good()) {
		printf("in stream bad!\n");
		return false;
	}

	return true;
}

void ArchiveFile::storeChunk(const Chunk &chunk) {
	// read directory entry
	vec3i64 cc = chunk.getCC();
	size_t x = cycle(cc[0], _region_size);
	size_t y = cycle(cc[1], _region_size);
	size_t z = cycle(cc[2], _region_size);
	size_t id = x + (_region_size * (y + (_region_size * z)));
	_file.seekg(_directory_offset + id * sizeof (DirectoryEntry));
	DirectoryEntry dir_entry;
	_file.read((char *)(&dir_entry), sizeof (DirectoryEntry));

	// encode the chunk
	const uint8 *blocks = chunk.getBlocks();
	// TODO fix extremely rare possibility of buffer overflow
	const size_t chunk_size = Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH;
	uint8 *const buffer = new uint8[chunk_size];
	uint8 cur_run_type = blocks[0];
	size_t cur_run_length = 1;
	uint8 *head = buffer;
	for (size_t i = 1; i < chunk_size; ++i) {
		uint8 next_block = blocks[i];
		if (cur_run_type == next_block && cur_run_length < 0xFF) {
			cur_run_length++;
			continue;
		}
		if (cur_run_length > 3 || cur_run_type == ESCAPE_CHAR) {
			*head++ = ESCAPE_CHAR;
			*head++ = cur_run_length;
			*head++ = cur_run_type;
		} else {
			for (size_t j = 0; j < cur_run_length; ++j)
				*head++ = cur_run_type;
		}
		cur_run_type = next_block;
		cur_run_length = 1;
	}

	if (cur_run_length > 3 || cur_run_type == ESCAPE_CHAR) {
		*head++ = ESCAPE_CHAR;
		*head++ = cur_run_length;
		*head++ = cur_run_type;
	} else {
		for (size_t j = 0; j < cur_run_length; ++j)
			*head++ = cur_run_type;
	}

	// store the chunk
	size_t store_size = (size_t) (head - buffer) * sizeof (uint8);
	if (dir_entry.offset != 0 && store_size <= dir_entry.size) {
		// chunk already existed, but the new one fits the old space
		_file.seekp(dir_entry.offset);
		//dir_entry.size = store_size
		/* We do not reset the size of the chunk, because a later write to this
		 * chunk might be bigger again and wants to reuse the space.  The
		 * reading code should be aware, that the chunk ends after a fixed
		 * number of blocks.  No marker or actual size is saved.
		 */
	} else {
		// chunk did not already exist or is now too big for the old space
		_file.seekg(0, ios_base::end);
		dir_entry.offset = (uint32) (_file.tellg());
		dir_entry.size = store_size;
		_file.seekp(_directory_offset + id * sizeof (DirectoryEntry));
		_file.write((char *)(&dir_entry), sizeof (DirectoryEntry));
		_file.seekp(0, ios_base::end);
	}
	_file.write((char *) buffer, store_size);
	_file.flush();
	delete[] buffer;

	if (!_file.good()) {
		LOG_ERROR(logger) << "Safe operation failed for chunk "
				<< cc[0] << " " << cc[1] << " "<< cc[2];
	} else {
		/*LOG_TRACE(logger) << "Safe operation successful for chunk "
				<< cc[0] << " " << cc[1] << " "<< cc[2]);*/
	}
}

ChunkArchive::~ChunkArchive() {
	clearHandles();
}

ChunkArchive::ChunkArchive(const char *str) :
	_path(str), _file_map(0, vec3i64HashFunc)
{
	using namespace boost::filesystem;
	path p(str);
	if (!exists(status(p))) {
		if (!create_directory(p)) {
			LOG_ERROR(logger) << "Could not create world directory";
		}
	} else {
		if (!is_directory(p)) {
			LOG_ERROR(logger) << "World path is not a directory";
		}
	}
}

void ChunkArchive::clearHandles() {
	for (auto iter : _file_map) {
		delete iter.second;
	}
	_file_map.clear();
}

Chunk *ChunkArchive::loadChunk(vec3i64 cc) {
	Chunk *chunk = new Chunk(cc);
	if (loadChunk(cc, *chunk)) {
		return chunk;
	} else {
		delete chunk;
		return nullptr;
	}
}

bool ChunkArchive::loadChunk(vec3i64 cc, Chunk &chunk) {
	ArchiveFile *archive_file = getArchiveFile(cc);
	return archive_file->loadChunk(cc, chunk);
}

bool ChunkArchive::loadChunk(Chunk &chunk) {
	return loadChunk(chunk.getCC(), chunk);
}

void ChunkArchive::storeChunk(const Chunk &chunk) {
	ArchiveFile *archive_file = getArchiveFile(chunk.getCC());
	archive_file->storeChunk(chunk);
}

ArchiveFile *ChunkArchive::getArchiveFile(vec3i64 cc) {
	static const uint REGION_SIZE = 16;
	vec3i64 rc;
	rc[0] = cc[0] / REGION_SIZE - (cc[0] < 0 ? 1 : 0);
	rc[1] = cc[1] / REGION_SIZE - (cc[1] < 0 ? 1 : 0);
	rc[2] = cc[2] / REGION_SIZE - (cc[2] < 0 ? 1 : 0);

	auto iter = _file_map.find(rc);
	if (iter == _file_map.end()) {
		char buffer[200];
		sprintf(buffer, "%" PRId64 "_%" PRId64 "_%" PRId64 ".region",
				rc[0], rc[1], rc[2]);
		std::string filename = _path + std::string(buffer);
		ArchiveFile *archive_file = new ArchiveFile(filename.c_str(), REGION_SIZE);
		_file_map.insert({rc, archive_file});
		return archive_file;
	} else {
		return iter->second;
	}
}
