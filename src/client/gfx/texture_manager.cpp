#include "texture_manager.hpp"

#include "shared/engine/logging.hpp"
#include "shared/block_utils.hpp"
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

uint8 TextureManager::scrambleBlockCoordinate(vec3i64 bc) {
	static const uint8 shuffle[256] = {
		0xa3,0xd7,0x09,0x83,0xf8,0x48,0xf6,0xf4,
		0xb3,0x21,0x15,0x78,0x99,0xb1,0xaf,0xf9,
		0xe7,0x2d,0x4d,0x8a,0xce,0x4c,0xca,0x2e,
		0x52,0x95,0xd9,0x1e,0x4e,0x38,0x44,0x28,
		0x0a,0xdf,0x02,0xa0,0x17,0xf1,0x60,0x68,
		0x12,0xb7,0x7a,0xc3,0xe9,0xfa,0x3d,0x53,
		0x96,0x84,0x6b,0xba,0xf2,0x63,0x9a,0x19,
		0x7c,0xae,0xe5,0xf5,0xf7,0x16,0x6a,0xa2,
		0x39,0xb6,0x7b,0x0f,0xc1,0x93,0x81,0x1b,
		0xee,0xb4,0x1a,0xea,0xd0,0x91,0x2f,0xb8,
		0x55,0xb9,0xda,0x85,0x3f,0x41,0xbf,0xe0,
		0x5a,0x58,0x80,0x5f,0x66,0x0b,0xd8,0x90,
		0x35,0xd5,0xc0,0xa7,0x33,0x06,0x65,0x69,
		0x45,0x00,0x94,0x56,0x6d,0x98,0x9b,0x76,
		0x97,0xfc,0xb2,0xc2,0xb0,0xfe,0xdb,0x20,
		0xe1,0xeb,0xd6,0xe4,0xdd,0x47,0x4a,0x1d,
		0x42,0xed,0x9e,0x6e,0x49,0x3c,0xcd,0x43,
		0x27,0xd2,0x07,0xd4,0xde,0xc7,0x67,0x18,
		0x89,0xcb,0x30,0x1f,0x8d,0xc6,0x8f,0xaa,
		0xc8,0x74,0xdc,0xc9,0x5d,0x5c,0x31,0xa4,
		0x70,0x88,0x61,0x2c,0x9f,0x0d,0x2b,0x87,
		0x50,0x82,0x54,0x64,0x26,0x7d,0x03,0x40,
		0x34,0x4b,0x1c,0x73,0xd1,0xc4,0xfd,0x3b,
		0xcc,0xfb,0x7f,0xab,0xe6,0x3e,0x5b,0xa5,
		0xad,0x04,0x23,0x9c,0x14,0x51,0x22,0xf0,
		0x29,0x79,0x71,0x7e,0xff,0x8c,0x0e,0xe2,
		0x0c,0xef,0xbc,0x72,0x75,0x6f,0x37,0xa1,
		0xec,0xd3,0x8e,0x62,0x8b,0x86,0x10,0xe8,
		0x08,0x77,0x11,0xbe,0x92,0x4f,0x24,0xc5,
		0x32,0x36,0x9d,0xcf,0xf3,0xa6,0xbb,0xac,
		0x5e,0x6c,0xa9,0x13,0x57,0x25,0xb5,0xe3,
		0xbd,0xa8,0x3a,0x01,0x05,0x59,0x2a,0x46
	};

	const uint8 *data = reinterpret_cast<uint8 *>(&bc);
	uint32 hash = 1;
	for (uint i = 0; i < sizeof (vec3i64); ++i) {
		hash *= 31;
		hash += data[i];
		hash ^= shuffle[ data[i] & 0xFF ];
	}
	return hash ^ (hash >> 8) ^ (hash >> 16) ^ (hash >> 24);
};

void TextureManager::getEdgeCoordinate(vec3i64 bc, uint8 dir,
	vec3i64 &vl, vec3i64 &vr, vec3i64 &vb, vec3i64 &vt)
{
	switch (dir) {
	case DIR_EAST:
		vl = bc + vec3i64(1, 0, 0);
		vr = bc + vec3i64(1, 1, 0);
		vb = bc + vec3i64(0, 0, 0);
		vt = bc + vec3i64(0, 0, 1);
		break;
	case DIR_WEST:
		vl = bc + vec3i64(0, 1, 0);
		vr = bc + vec3i64(0, 0, 0);
		vb = bc + vec3i64(0, 0, 0);
		vt = bc + vec3i64(0, 0, 1);
		break;
	case DIR_NORTH:
		vl = bc + vec3i64(1, 1, 0);
		vr = bc + vec3i64(0, 1, 0);
		vb = bc + vec3i64(0, 0, 0);
		vt = bc + vec3i64(0, 0, 1);
		break;
	case DIR_SOUTH:
		vl = bc + vec3i64(0, 0, 0);
		vr = bc + vec3i64(1, 0, 0);
		vb = bc + vec3i64(0, 0, 0);
		vt = bc + vec3i64(0, 0, 1);
		break;
	case DIR_UP:
		vl = bc + vec3i64(0, 0, 0);
		vr = bc + vec3i64(1, 0, 0);
		vb = bc + vec3i64(0, 0, 0);
		vt = bc + vec3i64(0, 1, 0);
		break;
	case DIR_DOWN:
		vl = bc + vec3i64(1, 0, 0);
		vr = bc + vec3i64(0, 0, 0);
		vb = bc + vec3i64(0, 0, 0);
		vt = bc + vec3i64(0, 1, 0);
		break;
	default:
		vl = bc;
		vr = bc;
		vb = bc;
		vt = bc;
		break;
	}
}

void TextureManager::getTextureCoords(int index, TextureType type, vec2f out[4]) {
	float x = 0.0;
	float y = 1.0;
	float w = 1.0;
	float h = -1.0;

	switch (type) {
	case TextureType::MULTI_x4:
	case TextureType::WANG_TILES:
		w /= 4.0;
		h /= 4.0;
		if (index >= 0) {
			x += w * float(index % 4);
			y += h * float(index / 4);
		}
		break;
	default:
		if (index >= 0) {
			w /= 16.0;
			h /= 16.0;
			x += w * float(index % 16);
			y += h * float(index / 16);
		}
		break;
	}

	out[0] = {x    , y    };
	out[1] = {x + w, y    };
	out[2] = {x + w, y + h};
	out[3] = {x    , y + h};
}

int TextureManager::getMultitileIndex(TextureType type, vec3i64 bc, uint8 dir) {
	int index = -1;

	if (type == TextureType::WANG_TILES) {
		vec3i64 vl, vr, vb, vt;
		getEdgeCoordinate(bc, dir, vl, vr, vb, vt);
		uint8 left, right, top, bot;
		switch (dir) {
		case DIR_DOWN:
			left  = scrambleBlockCoordinate(vl) & 0x10;
			right = scrambleBlockCoordinate(vr) & 0x10;
			bot   = scrambleBlockCoordinate(vb) & 0x20;
			top   = scrambleBlockCoordinate(vt) & 0x20;
			break;
		case DIR_UP:
			left  = scrambleBlockCoordinate(vl) & 0x04;
			right = scrambleBlockCoordinate(vr) & 0x04;
			bot   = scrambleBlockCoordinate(vb) & 0x08;
			top   = scrambleBlockCoordinate(vt) & 0x08;
			break;
		default:
			left  = scrambleBlockCoordinate(vl) & 0x01;
			right = scrambleBlockCoordinate(vr) & 0x01;
			bot   = scrambleBlockCoordinate(vb) & 0x02;
			top   = scrambleBlockCoordinate(vt) & 0x02;
			break;
		}

		index = 0;

		if      ( left && !right) index += 1;
		else if (!left && !right) index += 2;
		else if (!left &&  right) index += 3;
		if      ( bot  && !top)   index += 4;
		else if (!bot  && !top)   index += 8;
		else if (!bot  &&  top)   index += 12;
	} else if (type == TextureType::MULTI_x4) {
		vec3i64 vl, vr, vb, vt;
		getEdgeCoordinate(bc, dir, vl, vr, vb, vt);

		auto clamp = [](int64 i, int64 a, int64 b) -> int64 {
			auto tmp = (i - a) % (b - a);
			return tmp < 0 ? tmp + b : tmp + a;
		};

		index = 0;

		index += (int) clamp((vr - vl) * vl, 0, 4);
		index += (int) clamp((vt - vb) * vb, 0, 4) * 4;
	}

	return index;
}
