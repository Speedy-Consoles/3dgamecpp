#ifndef TEXTURE_LOADER_HPP_
#define TEXTURE_LOADER_HPP_

#include <string>
#include <vector>
#include <memory>

#include <SDL2/SDL.h>

#include "shared/engine/std_types.hpp"

class Client;
class TextureManager;
class BlockManager;

class TextureLoader {
	const char *path;
	FILE *f = nullptr;
	const BlockManager *bm = nullptr;
	TextureManager *tm = nullptr;

public:

	struct ParsingError {
		int row, col;
		std::string error;
	};

	TextureLoader(const char *path, const BlockManager *bm, TextureManager *tm);
	~TextureLoader();

	int load();

private:

	enum TokenId {
		TOK_EOF,
		TOK_LBRACE,
		TOK_RBRACE,
		TOK_EQUAL,
		TOK_STRING,
		TOK_NUMERAL,
		TOK_IDENT,
	};

	struct Token {
		TokenId id;
		int pos;
		int row;
		int col;
		std::string str;
		union {
			long i;
			const char *err;
		};
	};

	int ch = 0;
	int pos = 0;
	int row = 0;
	int col = 0;
	void getNextChar();

	Token tok;
	void getNextToken();
};

enum class TextureType {
	SINGLE_TEXTURE,
	WANG_TILES,
	MULTI_x4,
};

struct TextureLoadEntry {
	int id;
	TextureType type;
	uint8 dir_mask;
	int x, y, w, h;
};

// this makes sure that smart pointers of SDL_Surfaces delete the object properly
namespace std {
	template<>
	void default_delete<SDL_Surface>::operator()(SDL_Surface* s) const;
}

#endif //TEXTURE_LOADER_HPP_
