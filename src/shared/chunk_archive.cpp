#include "chunk_archive.hpp"

#include <cstring>

#include <boost/filesystem.hpp>

#include "engine/math.hpp"
#include "engine/logging.hpp"

#include "block_utils.hpp"
#include "chunk_compression.hpp"

using namespace std;

static logging::Logger logger("io");

static const uint8 MAGIC[4] = { 0x59, 0x97, 0x22, 0xDF };

static const int32 ENDIANESS_BYTES = 0x01020304;
static const int32 ENDIANESS_BYTES_FLIPPED = 0x04030201;

static const uint HEAP_BLOCK_SIZE = 256;

static const int32 RECENT_HEADER_VERSION = 3;

class ArchiveFile {
private:

	PACKED(
	struct Header {
		uint8 magic[4];
		int32 endianess_bytes;
		int32 version;
		uint32 dir_size;
		uint32 directory_offset;
		uint16 heap_block_size;
		uint8 reserved[10];
	});

	enum Layout {
		LAYOUT_PLAIN      = 0x0000,
		LAYOUT_DIFF       = 0x0001,
		LAYOUT_RLE        = 0x0002,
		LAYOUT_ZLIB       = 0x0004,
		LAYOUT_ENC_MASK   = 0x0007,
		LAYOUT_VISIBILITY = 0x0008,
		LAYOUT_EMPTY      = 0x8000,
	};

	PACKED(
	struct DirectoryEntry {
		uint32 offset;
		uint16 size;
		uint16 flags;
		uint32 revision;
		uint16 visibility;
		uint8  reserved[2];
	});

public:
	enum Endianess {
		UNKNOWN = -1,
		NATIVE,
		FLIPPED
	};

	~ArchiveFile();
	ArchiveFile(const char *, uint size = 16);

	ArchiveFile() = delete;
	ArchiveFile(const ArchiveFile &) = delete;
	ArchiveFile(ArchiveFile &&) = delete;

	ArchiveFile &operator = (const ArchiveFile &) = delete;
	ArchiveFile &operator = (ArchiveFile &&) = delete;

	Time getLastAccess() const { return _last_access; }
	
	void loadHeader();
	void loadDirectory();
	void initialize();

	bool hasChunk(vec3i64, uint32 *);
	bool loadChunk(Chunk *);
	void storeChunk(const Chunk &);

	int getFileSize();
	int getUsedChunkBytes();
	int getTotalChunkBytes();
	float getChunkFragmentation();

private:
	size_t getChunkHeapStart();

	std::fstream _file;
	const uint _region_size;
	Time _last_access;
	std::string _filename;
	bool _good = true;

	Header _header;
	std::vector<DirectoryEntry> _dir;
};

ArchiveFile::~ArchiveFile() {
	if(_file.is_open()) _file.close();
}

ArchiveFile::ArchiveFile(const char *filename, uint region_size) :
	_region_size(region_size), _last_access(getCurrentTime()), _filename(filename)
{
	_file.open(_filename, ios_base::in | ios_base::out | ios_base::binary);
	if (!_file.is_open()) {
		// file might have not existed, try to create it
		_file.clear();
		_file.open(_filename, ios_base::out);
		if (_file.is_open()) {
			_file.close();
		}
		// try again
		_file.clear();
		_file.open(_filename, ios_base::in | ios_base::out | ios_base::binary);
		if (!_file.is_open()) {
			LOG_ERROR(logger) << "Could not open ArchiveFile '" << _filename << "'";
			_good = false;
			return;
		} else {
			int c = _file.get();
			if (c == EOF) {
				// file was empty, we can safely nuke it (we probably created it)
				initialize();
			} else {
				LOG_ERROR(logger) << "ArchiveFile '" << _filename << "' had content after creation";
				_file.close();
				_good = false;
				return;
			}
		}
	}

	loadHeader();
	if (!_good) {
		LOG_ERROR(logger) << "ArchiveFile '" << _filename << "' had bad header";
		_file.close();
		_good = false;
		return;
	}

	if (_header.version != RECENT_HEADER_VERSION) {
		LOG_ERROR(logger) << "ArchiveFile '" << _filename << "' had unknown version ("
				<< _header.version << ")";
		_file.close();
		_good = false;
		return;
	}

	loadDirectory();
}

void ArchiveFile::loadHeader() {
	_file.clear();
	_file.seekg(0);
	_file.read((char *)(&_header), sizeof(Header));
	if (_file.eof()) {
		LOG_ERROR(logger) << "Archive file '" << _filename << "' ended abruptly";
		_good = false;
		return;
	}

	if (memcmp(_header.magic, MAGIC, sizeof(MAGIC)) != 0) {
		LOG_ERROR(logger) << "Archive file '" << _filename << "' had wrong magic (0x"
				<< std::hex << std::uppercase << (uint) _header.magic[0]
				<< (uint) _header.magic[1]
				<< (uint) _header.magic[2]
				<< (uint) _header.magic[3]
				<< " instead of 0x"
				<< std::hex << std::uppercase << (uint) MAGIC[0]
				<< (uint) MAGIC[1]
				<< (uint) MAGIC[2]
				<< (uint) MAGIC[3]
				<< ")";
		_good = false;
		return;
	}

	if (_header.endianess_bytes != ENDIANESS_BYTES) {
		LOG_ERROR(logger) << "ArchiveFile '" << _filename << "' had wrong endianess";
		_good = false;
		return;
	}

	if (_header.dir_size != _region_size * _region_size * _region_size) {
		LOG_ERROR(logger) << "ArchiveFile '" << _filename << "' had wrong size ("
				<< _header.dir_size << " instead of "
				<< _region_size * _region_size * _region_size << ")";
		_good = false;
		return;
	}

	_dir.clear();
	_dir.resize(_header.dir_size);
	memset((char *)_dir.data(), 0, _header.dir_size * sizeof(DirectoryEntry));
}

void ArchiveFile::loadDirectory() {
	_file.seekg(_header.directory_offset);
	_file.read((char *)(_dir.data()), _header.dir_size * sizeof (DirectoryEntry));
}

void ArchiveFile::initialize() {
	_file.clear();
	_file.seekp(0);

	memset((char *)&_header, 0, sizeof(Header));
	memcpy(_header.magic, MAGIC, sizeof (MAGIC));
	_header.endianess_bytes = ENDIANESS_BYTES;
	_header.version = RECENT_HEADER_VERSION;
	uint num_chunks = _region_size * _region_size * _region_size;
	_header.dir_size = num_chunks;
	_header.directory_offset = sizeof (Header);
	_header.heap_block_size = HEAP_BLOCK_SIZE;

	// write header and empty directory
	_file.write((char *)(&_header), sizeof (Header));
	const size_t total_directory_bytes = num_chunks * sizeof(DirectoryEntry);
	for (uint i = 0; i < total_directory_bytes; ++i) {
		_file.put(0);
	}
	_file.flush();

	if (!_file.good()) {
		LOG_ERROR(logger) << "Could not initialize ArchiveFile '" << _filename << "'";
		_good = false;
		return;
	}

	_dir.clear();
	_dir.resize(_header.dir_size);
	memset((char *)_dir.data(), 0, _header.dir_size * sizeof(DirectoryEntry));
}

bool ArchiveFile::hasChunk(vec3i64 cc, uint32 *revision) {
	if (!_good) return false;
	size_t x = cycle(cc[0], _region_size);
	size_t y = cycle(cc[1], _region_size);
	size_t z = cycle(cc[2], _region_size);
	size_t id = x + (_region_size * (y + (_region_size * z)));
	const DirectoryEntry &dir_entry = _dir[id];
	bool has_chunk = dir_entry.size != 0 || dir_entry.flags != 0;
	if (revision != nullptr && has_chunk)
		*revision = dir_entry.revision;
	return has_chunk;
}

bool ArchiveFile::loadChunk(Chunk *chunk) {
	if (!_good) return false;

	_last_access = getCurrentTime();

	// get entry from directory
	vec3i64 cc = chunk->getCC();
	size_t x = cycle(cc[0], _region_size);
	size_t y = cycle(cc[1], _region_size);
	size_t z = cycle(cc[2], _region_size);
	size_t id = x + (_region_size * (y + (_region_size * z)));
	const DirectoryEntry &dir_entry = _dir[id];

	if (dir_entry.size == 0 && dir_entry.flags == 0) {
		return false;
	}
	
	chunk->initRevision(dir_entry.revision);
	if (dir_entry.flags == LAYOUT_EMPTY) {
		uint8 *blocks = chunk->getBlocksForInit();
		memset((char *)blocks, 0, Chunk::SIZE * sizeof(uint8));
		chunk->initNumAirBlocks(Chunk::SIZE);
		chunk->initPassThroughs(0x7FFF);
		chunk->finishInitialization();
		return true;
	}

	_file.seekg(getChunkHeapStart() + dir_entry.offset * _header.heap_block_size);

	if ((dir_entry.flags & LAYOUT_ENC_MASK) == LAYOUT_RLE) {
		decodeBlocks_RLE(&_file, chunk->getBlocksForInit());
	} else if ((dir_entry.flags & LAYOUT_ENC_MASK) == LAYOUT_PLAIN) {
		decodeBlocks_PLAIN(&_file, chunk->getBlocksForInit());
	} else {
		LOG_ERROR(logger) << "Chunk Layout " << dir_entry.flags << " unsupported";
		return false;
	}

	if (!_file.good()) {
		LOG_ERROR(logger) << "Bad in-stream";
		return false;
	}

	if (dir_entry.flags & LAYOUT_VISIBILITY)
		chunk->initPassThroughs(dir_entry.visibility);

	chunk->finishInitialization();

	return true;
}

void ArchiveFile::storeChunk(const Chunk &chunk) {
	if (!_good) return;

	_last_access = getCurrentTime();

	vec3i64 cc = chunk.getCC();
	size_t x = cycle(cc[0], _region_size);
	size_t y = cycle(cc[1], _region_size);
	size_t z = cycle(cc[2], _region_size);
	size_t id = x + (_region_size * (y + (_region_size * z)));
	DirectoryEntry &dir_entry = _dir[id];

	dir_entry.revision = chunk.getRevision();

	if (chunk.isEmpty()) {
		dir_entry.offset = 0;
		dir_entry.size = 0;
		dir_entry.visibility = 0;
		dir_entry.flags = LAYOUT_EMPTY;
	} else {

		uint8 *const buffer = new uint8[Chunk::SIZE];
		int bytes_written;

		// try RLE encoding
		bytes_written = encodeBlocks_RLE(chunk.getBlocks(), buffer, Chunk::SIZE);
		if (bytes_written < 0) {
			LOG_ERROR(logger) << "Chunk (" << cc << ") could not be written";
			return;
		}
		dir_entry.flags = LAYOUT_RLE;

		// use plain encoding if we didn't compress the chunk enough
		if ((uint) bytes_written / _header.heap_block_size >= Chunk::SIZE / _header.heap_block_size) {
			bytes_written = encodeBlocks_PLAIN(chunk.getBlocks(), buffer, Chunk::SIZE);
			if (bytes_written < 0) {
				LOG_ERROR(logger) << "Chunk (" << cc << ") could not be written";
				return;
			}
			dir_entry.flags = LAYOUT_PLAIN;
		}

		uint num_blocks = (bytes_written - 1) / _header.heap_block_size + 1;
		if (num_blocks > dir_entry.size) {
			//LOG_DEBUG(logger) << "Resized Chunk (" << cc << ")";
			_file.seekp(0, ios_base::end);
			size_t file_end = (size_t) _file.tellp();
			size_t chunk_heap_size = file_end - getChunkHeapStart();
			size_t start;
			if (chunk_heap_size > 0)
				start = (chunk_heap_size - 1) / _header.heap_block_size + 1;
			else
				start = 0;
			dir_entry.offset = (uint32) start;
			dir_entry.size = num_blocks;
		}

		_file.seekp(getChunkHeapStart() + dir_entry.offset * _header.heap_block_size);
		_file.write((char *) buffer, bytes_written);
		_file.flush();

		delete[] buffer;

		if (chunk.isVisual()) {
			dir_entry.flags |= LAYOUT_VISIBILITY;
			dir_entry.visibility = chunk.getPassThroughs();
		} else {
			dir_entry.visibility = 0;
		}
	}

	_file.seekp(_header.directory_offset + id * sizeof (DirectoryEntry));
	_file.write((char *)(&dir_entry), sizeof (DirectoryEntry));
	_file.flush();

	if (!_file.good()) {
		LOG_ERROR(logger) << "Safe operation failed for chunk "
				<< cc[0] << " " << cc[1] << " "<< cc[2];
	}
}

int ArchiveFile::getFileSize() {
	using namespace boost::filesystem;
	return (int) file_size(path(_filename));
}

int ArchiveFile::getUsedChunkBytes() {
	size_t blocks = 0;
	for (auto &entry : _dir) {
		blocks += entry.size;
	}
	return (int) blocks * _header.heap_block_size;
}

int ArchiveFile::getTotalChunkBytes() {
	size_t blocks = 0;
	for (auto &entry : _dir) {
		blocks = std::max(blocks, (size_t) entry.size + entry.offset);
	}
	return (int) blocks * _header.heap_block_size;
}

float ArchiveFile::getChunkFragmentation() {
	size_t used = getUsedChunkBytes();
	size_t total = getTotalChunkBytes();
	return total > 0 ? (float) (total - used) / total : 0.0f;
}

size_t ArchiveFile::getChunkHeapStart() {
	return _header.directory_offset + _header.dir_size * sizeof(DirectoryEntry);
}

ChunkArchive::~ChunkArchive() {
	clean();
}

ChunkArchive::ChunkArchive(const char *str) :
	_path(str), _file_map(0, vec3i64HashFunc)
{
	using namespace boost::filesystem;
	path p(str);
	if (!exists(status(p))) {
		create_directories(p);
	} else {
		if (!is_directory(p)) {
			LOG_ERROR(logger) << "World path is not a directory";
			return;
		}
	}

	size_t bytes = 0;
	size_t used_bytes = 0;
	size_t total_bytes = 0;

	directory_iterator iter(str);
	directory_iterator end;
	while (iter != end) {
		if (exists(iter->path()) && is_regular_file(iter->path())) {
			ArchiveFile af(iter->path().string().c_str());
			bytes += af.getFileSize();
			used_bytes += af.getUsedChunkBytes();
			total_bytes += af.getTotalChunkBytes();
		}
		++iter;
	}

	LOG_INFO(logger) << "Chunk archive '" << str << "' has " << bytes / 1024 / 1024 << " MB";
	LOG_INFO(logger) << "Chunk archive '" << str << "' uses "
			<< used_bytes / 1024 / 1024 << " MB of " << total_bytes / 1024 / 1024 << " MB of space";
}

bool ChunkArchive::hasChunk(vec3i64 cc, uint32 *revision) {
	ArchiveFile *archive_file = getArchiveFile(cc);
	return archive_file->hasChunk(cc, revision);
}

bool ChunkArchive::loadChunk(Chunk *chunk) {
	ArchiveFile *archive_file = getArchiveFile(chunk->getCC());
	return archive_file->loadChunk(chunk);
}

void ChunkArchive::storeChunk(const Chunk &chunk) {
	ArchiveFile *archive_file = getArchiveFile(chunk.getCC());
	archive_file->storeChunk(chunk);
}

void ChunkArchive::clean(Time t) {
	int num_cleaned = 0;
	Time now = getCurrentTime();
	auto iter = _file_map.begin();
	while (iter != _file_map.end()) {
		bool should_delete = now - iter->second->getLastAccess() > t;
		if (should_delete) {
			delete iter->second;
			iter = _file_map.erase(iter);
			++num_cleaned;
		} else {
			++iter;
		}
	}
	if (num_cleaned)
		LOG_DEBUG(logger) << "Cleaned " << num_cleaned << " file handles";
}

ArchiveFile *ChunkArchive::getArchiveFile(vec3i64 cc) {
	static const uint REGION_SIZE = 16;
	vec3i64 rc;
	rc[0] = cc[0] / REGION_SIZE - (cc[0] < 0 ? 1 : 0);
	rc[1] = cc[1] / REGION_SIZE - (cc[1] < 0 ? 1 : 0);
	rc[2] = cc[2] / REGION_SIZE - (cc[2] < 0 ? 1 : 0);

	auto iter = _file_map.find(rc);
	if (iter != _file_map.end()) {
		return iter->second;
	} else {
		clean(seconds(10));
		char buffer[200];
		sprintf(buffer, "%" PRId64 "_%" PRId64 "_%" PRId64 ".region",
				rc[0], rc[1], rc[2]);
		std::string filename = _path + std::string(buffer);
		ArchiveFile *archive_file = new ArchiveFile(filename.c_str(), REGION_SIZE);
		_file_map.insert({rc, archive_file});
		return archive_file;
	}
}
