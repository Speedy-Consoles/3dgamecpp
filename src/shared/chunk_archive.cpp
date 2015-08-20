#include "chunk_archive.hpp"

#include <cstring>

#include <boost/filesystem.hpp>

#include "engine/math.hpp"
#include "engine/logging.hpp"

#include "block_utils.hpp"

using namespace std;

static logging::Logger logger("io");

static const char MAGIC[4] = {89, -105, 34, -33};

static const int32 ENDIANESS_BYTES = 0x01020304;
static const int32 ENDIANESS_BYTES_FLIPPED = 0x04030201;

static const uint8 ESCAPE_CHAR = (uint8) (-1);

class ArchiveFile {
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
	
	void loadDirectory();
	void updateFormat();
	void initialize();

	bool hasChunk(vec3i64);

	bool loadChunk(vec3i64 cc, Chunk &c) { c.setCC(cc); return loadChunk(c); }
	bool loadChunk(Chunk &c);
	void decodeChunk(Chunk &c);
	void decodeChunk_LEGACY_RLE(Chunk &c);
	
	void storeChunk(const Chunk &);
	int encodeChunk(const Chunk &, uint8 *, size_t);
	int encodeChunk_LEGACY_RLE(const Chunk &, uint8 *, size_t);

	int getFileSize();
	int getUsedChunkBytes();
	int getTotalChunkBytes();
	float getChunkFragmentation();

private:
	std::fstream _file;
	const uint _region_size;
	Time _last_access;
	std::string _filename;

	PACKED(
	struct Header {
		char magic[4];
		int32 endianess_bytes;
		int32 version;
		uint32 num_chunks;
		uint32 directory_offset;
	});

	PACKED(
	struct DirectoryEntry {
		uint32 offset;
		uint32 size;
	});

	Header _header;
	std::vector<DirectoryEntry> _dir;
};

ArchiveFile::~ArchiveFile() {
	_file.close();
}

ArchiveFile::ArchiveFile(const char *filename, uint region_size) :
	_region_size(region_size), _last_access(getCurrentTime()), _filename(filename)
{
	_file.open(filename, ios_base::in | ios_base::out | ios_base::binary);
	if (_file.fail()) {
		_file.open(filename, ios_base::out);
		_file.close();
		_file.open(filename, ios_base::in | ios_base::out | ios_base::binary);
		if (!_file.good()) {
			LOG_ERROR(logger) << "Could not open Archive file '" << filename << "'";
		}
	}

	bool all_ok;

	_file.read((char *)(&_header), sizeof (Header));
	if (_file.eof()) {
		_file.clear();
		all_ok = false;
	} else {
		bool is_magic_ok = memcmp(_header.magic, MAGIC, 4 * sizeof (char)) == 0;
		bool is_endianess_ok = _header.endianess_bytes == ENDIANESS_BYTES;
		bool is_version_ok = _header.version == 1;
		bool is_size_ok = _header.num_chunks == _region_size * _region_size * _region_size;

		all_ok = is_magic_ok && is_endianess_ok && is_version_ok && is_size_ok;
	}

	if (all_ok) {
		loadDirectory();
	} else {
		initialize();
	}
}

void ArchiveFile::loadDirectory() {
	_file.seekg(_header.directory_offset);
	_dir.resize(_header.num_chunks);
	_file.read((char *)(_dir.data()), _header.num_chunks * sizeof (DirectoryEntry));
}

void ArchiveFile::initialize() {
	memcpy(_header.magic, MAGIC, 4 * sizeof (char));
	_header.endianess_bytes = ENDIANESS_BYTES;
	_header.version = 0x0001;
	uint num_chunks = _region_size * _region_size * _region_size;
	_header.num_chunks = num_chunks;
	_header.directory_offset = sizeof (Header);

	// write header
	_file.seekp(0);
	_file.write((char *)(&_header), sizeof (Header));

	// zero the directory
	_dir.clear();
	_dir.resize(num_chunks);
	memset(_dir.data(), 0, num_chunks * sizeof(DirectoryEntry));

	const size_t total_directory_bytes = num_chunks * sizeof(DirectoryEntry);
	for (uint i = 0; i < total_directory_bytes; ++i) {
		_file.put(0);
	}
	_file.flush();
}

bool ArchiveFile::hasChunk(vec3i64 cc) {
	size_t x = cycle(cc[0], _region_size);
	size_t y = cycle(cc[1], _region_size);
	size_t z = cycle(cc[2], _region_size);
	size_t id = x + (_region_size * (y + (_region_size * z)));
	return _dir[id].offset != 0;
}

bool ArchiveFile::loadChunk(Chunk &chunk) {
	_last_access = getCurrentTime();

	// read directory entry
	vec3i64 cc = chunk.getCC();
	size_t x = cycle(cc[0], _region_size);
	size_t y = cycle(cc[1], _region_size);
	size_t z = cycle(cc[2], _region_size);
	size_t id = x + (_region_size * (y + (_region_size * z)));
	const DirectoryEntry &dir_entry = _dir[id];

	// seek there
	if (dir_entry.offset == 0) {
		return false;
	} else {
		_file.seekg(dir_entry.offset);
	}

	// load and decode the chunk
	decodeChunk(chunk);

	if (!_file.good()) {
		LOG_ERROR(logger) << "Bad in-stream";
		return false;
	}

	chunk.finishInitialization();

	return true;
}

void ArchiveFile::decodeChunk(Chunk &chunk) {
	switch (_header.version) {
	case 1:
		decodeChunk_LEGACY_RLE(chunk);
		break;
	default:
		LOG_ERROR(logger) << "ArchiveFile version " << _header.version << "not supported";
		break;
	}
}

void ArchiveFile::decodeChunk_LEGACY_RLE(Chunk &chunk) {
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
				if (index >= chunk_size) {
					LOG_ERROR(logger) << "Block data exceeded Chunk size";
					break;
				}
				chunk.initBlock(index++, block_type);
			}
		} else {
			chunk.initBlock(index++, next_block);
		}
	}
}

void ArchiveFile::storeChunk(const Chunk &chunk) {
	_last_access = getCurrentTime();

	vec3i64 cc = chunk.getCC();
	size_t x = cycle(cc[0], _region_size);
	size_t y = cycle(cc[1], _region_size);
	size_t z = cycle(cc[2], _region_size);
	size_t id = x + (_region_size * (y + (_region_size * z)));

	// encode the chunk
	// TODO fix extremely rare possibility of buffer overflow
	const size_t chunk_size = Chunk::WIDTH * Chunk::WIDTH * Chunk::WIDTH;
	uint8 *const buffer = new uint8[chunk_size];
	int bytes_written = encodeChunk(chunk, buffer, chunk_size);
	if (bytes_written < 0) {
		LOG_ERROR(logger) << "Chunk (" << cc << ") could not be written";
		return;
	}

	// store the chunk
	DirectoryEntry &dir_entry = _dir[id];
	size_t store_size = (size_t) bytes_written * sizeof (uint8);

	if (_header.version == 1) {
		if (dir_entry.offset != 0 && store_size <= dir_entry.size) {
			// chunk already existed, but the new one fits the old space
			_file.seekp(dir_entry.offset);
			//dir_entry.size = store_size
		} else {
			// chunk did not already exist or is now too big for the old space
			_file.seekg(0, ios_base::end);
			dir_entry.offset = (uint32) (_file.tellg());
			dir_entry.size = store_size;
			_file.seekp(_header.directory_offset + id * sizeof (DirectoryEntry));
			_file.write((char *)(&dir_entry), sizeof (DirectoryEntry));
			_file.seekp(0, ios_base::end);
		}
		_file.write((char *) buffer, store_size);
		_file.flush();
	}

	delete[] buffer;

	if (!_file.good()) {
		LOG_ERROR(logger) << "Safe operation failed for chunk "
				<< cc[0] << " " << cc[1] << " "<< cc[2];
	}
}

int ArchiveFile::encodeChunk(const Chunk &chunk, uint8 *buffer, size_t size) {
	switch (_header.version) {
	case 1:
		return encodeChunk_LEGACY_RLE(chunk, buffer, size);
	default:
		LOG_ERROR(logger) << "ArchiveFile version " << _header.version << " not supported";
		return -1;
	}
}

int ArchiveFile::encodeChunk_LEGACY_RLE(const Chunk &chunk, uint8 *buffer, size_t size) {
	const uint8 *blocks = chunk.getBlocks();
	uint8 cur_run_type = blocks[0];
	size_t cur_run_length = 1;
	uint8 *head = buffer;

	auto finishRun = [&]() -> int {
		if (cur_run_length > 3 || cur_run_type == ESCAPE_CHAR) {
			if ((size_t) (head - buffer + 3) <= size) {
				*head++ = ESCAPE_CHAR;
				*head++ = cur_run_length;
				*head++ = cur_run_type;
			} else {
				return -1;
			}
		} else {
			if (head - buffer + cur_run_length <= size) {
				for (size_t j = 0; j < cur_run_length; ++j)
					*head++ = cur_run_type;
			} else {
				return -1;
			}
		}
		return 0;
	};

	for (size_t i = 1; i < size; ++i) {
		uint8 next_block = blocks[i];
		if (cur_run_type == next_block && cur_run_length < 0xFF) {
			cur_run_length++;
			continue;
		}
		if (finishRun() != 0) return -1;
		cur_run_type = next_block;
		cur_run_length = 1;
	}

	if (finishRun() != 0) return -1;
	return head - buffer;
}

int ArchiveFile::getFileSize() {
	using namespace boost::filesystem;
	return file_size(path(_filename));
}

int ArchiveFile::getUsedChunkBytes() {
	size_t bytes = 0;
	for (auto entry : _dir) {
		if (entry.offset > 0)
			bytes += entry.size;
	}
	return (int) bytes;
}

int ArchiveFile::getTotalChunkBytes() {
	size_t bytes = 0;
	for (auto entry : _dir) {
		if (entry.offset > 0)
			bytes = std::max(bytes, (size_t) entry.size + entry.offset);
	}
	return (int) bytes;
}

float ArchiveFile::getChunkFragmentation() {
	size_t used = getUsedChunkBytes();
	size_t total = getTotalChunkBytes();
	return total > 0 ? (float) (total - used) / total : 0.0f;
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
		if (!create_directories(p)) {
			LOG_ERROR(logger) << "Could not create world directory";
		}
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

	size_t megabytes = bytes / 1024 / 1024;
	LOG_INFO(logger) << "Chunk archive '" << str << "' has " << megabytes << " MB";
	
	float frag_mean = (float) (total_bytes - used_bytes) / total_bytes;
	LOG_INFO(logger) << "Chunk archive '" << str << "' uses "
			<< used_bytes / 1024 / 1024 << " MB of " << total_bytes / 1024 / 1024 << " MB of space";
}

bool ChunkArchive::hasChunk(vec3i64 cc) {
	ArchiveFile *archive_file = getArchiveFile(cc);
	return archive_file->hasChunk(cc);
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

void ChunkArchive::clean(Time t) {
	int num_cleaned = 0;
	Time now = getCurrentTime();
	auto iter = _file_map.begin();
	while (iter != _file_map.end()) {
		bool should_delete = now - iter->second->getLastAccess() > t;
		if (should_delete) {
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
