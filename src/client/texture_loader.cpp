#include "texture_loader.hpp"
#include "game/block_manager.hpp"

#include <SDL2/SDL_image.h>
#include <memory>
#include <string>

#include "engine/logging.hpp"

struct ParsingError {
	int row, col;
	std::string error;
};

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

class TextureLoader {
	const char *path;
	FILE *f = nullptr;
	const BlockManager *bm = nullptr;
	AbstractTextureManager *tm = nullptr;

public:
	TextureLoader(const char *path, const BlockManager *bm, AbstractTextureManager *tm);
	~TextureLoader();

	int load();

private:
	int ch = 0;
	int pos = 0;
	int row = 0;
	int col = 0;
	void getNextChar();

	Token tok;
	void getNextToken();
};

TextureLoader::TextureLoader(const char *path, const BlockManager *bm, AbstractTextureManager *tm) :
	path(path), bm(bm), tm(tm)
{
	f = fopen(path, "r");
}

TextureLoader::~TextureLoader() {
	if (f)
		fclose(f);
}

int TextureLoader::load() {
	std::vector<TextureEntry> entries;
	TextureEntry entry = {-1, TextureType::SINGLE_TEXTURE, 0, 0, 0, 0};
	Token key;

	struct SurfaceDeleter { void operator () (SDL_Surface *s) { SDL_FreeSurface(s); } };
	std::unique_ptr<SDL_Surface, SurfaceDeleter> img;
	int width = 1;
	int height = 1;

	auto finishTexture = [&]() {
		this->tm->add(img.get(), entries);
		entries.clear();
		entry.w = 0;
		entry.h = 0;
		img.reset();
		width = 1;
		height = 1;
	};

	auto finishBlock = [&]() {
		entries.push_back(entry);
		entry.id = -1;
		entry.type = TextureType::SINGLE_TEXTURE;
		entry.x = 0;
		entry.y = 0;
	};

	enum class State {
		EXPECT_TEXTURE,
		TEX,
		TEX_EXPECT_EQUAL,
		TEX_EXPECT_VALUE,
		EXPECT_BLOCK,
		BLOCK,
		BLOCK_EXPECT_EQUAL,
		BLOCK_EXPECT_VALUE,
		RECOVER_AT_NEXT_TEXTURE,
		RECOVER_AT_NEXT_BLOCK,
	};

	ch = getc(f);
	State state = State::EXPECT_TEXTURE;
	while (true) {
		getNextToken();

		switch (state) {
		case State::EXPECT_TEXTURE:
			if (tok.id == TOK_EOF) {
				finishTexture();
				return 0;
			} else if (tok.str == "texture") {
				state = State::TEX;
			} else {
				throw ParsingError{row, col, "Expected 'texture' or EOF"};
			}
			continue;
		case State::TEX:
			if (tok.id == TOK_EOF) {
				throw ParsingError{row, col, "Unexpected EOF"};
			} else if (tok.str == "{") {
				state = State::EXPECT_BLOCK;
			} else if (tok.id == TOK_IDENT) {
				key = tok;
				state = State::TEX_EXPECT_EQUAL;
			} else {
				throw ParsingError{row, col, "Expected identifier or '{'"};
			}
			continue;
		case State::TEX_EXPECT_EQUAL:
			if (tok.id == TOK_EOF) {
				throw ParsingError{row, col, "Unexpected EOF"};
			} else if (tok.str == "=") {
				state = State::TEX_EXPECT_VALUE;
			} else {
				throw ParsingError{row, col, "Expected '='"};
			}
			continue;
		case State::TEX_EXPECT_VALUE:
			if (tok.id == TOK_EOF) {
				throw ParsingError{row, col, "Unexpected EOF"};
			}

			if (key.str == "file") {
				img = std::unique_ptr<SDL_Surface, SurfaceDeleter>(IMG_Load(tok.str.c_str()));
				if (!img) {
					LOG(ERROR, "File '" << tok.str << "' could not be loaded");
					state = State::RECOVER_AT_NEXT_TEXTURE;
				}
			} else if (key.str == "width") {
				if (tok.id == TOK_NUMERAL) {
					width = tok.i;
					entry.w = img->w / width;
				} else {
					throw ParsingError{tok.row, tok.col, "Expected numeral"};
				}
			} else if (key.str == "height") {
				if (tok.id == TOK_NUMERAL) {
					height = tok.i;
					entry.h = img->h / height;
				} else {
					throw ParsingError{tok.row, tok.col, "Expected numeral"};
				}
			} else {
				throw ParsingError{key.row, key.col, "Unknown attribute"};
			}

			state = State::TEX;
			continue;
		case State::EXPECT_BLOCK:
			if (tok.id == TOK_EOF) {
				throw ParsingError{row, col, "Unexpected EOF"};
			} else if (tok.str == "}") {
				state = State::EXPECT_TEXTURE;
			} else if (tok.str == "block") {
				state = State::BLOCK;
			} else {
				throw ParsingError{row, col, "Expected 'block'"};
			}
			continue;
		case State::BLOCK:
			if (tok.id == TOK_EOF) {
				throw ParsingError{row, col, "Unexpected EOF"};
			} else if (tok.str == "block") {
				finishBlock();
			} else if (tok.str == "}") {
				finishBlock();
				finishTexture();
				state = State::EXPECT_TEXTURE;
			} else if (tok.id == TOK_IDENT) {
				key = tok;
				state = State::BLOCK_EXPECT_EQUAL;
			}
			continue;
		case State::BLOCK_EXPECT_EQUAL:
			if (tok.id == TOK_EOF) {
				throw ParsingError{row, col, "Unexpected EOF"};
			} else if (tok.str == "=") {
				state = State::BLOCK_EXPECT_VALUE;
			} else {
				throw ParsingError{row, col, "Expected '='"};
			}
			continue;
		case State::BLOCK_EXPECT_VALUE:
			if (tok.id == TOK_EOF) {
				throw ParsingError{row, col, "Unexpected EOF"};
			}

			if (key.str == "name") {
				if (tok.id == TOK_IDENT) {
					int id = bm->getBlockId(tok.str);
					if (id > 0) {
						entry.id = id;
						state = State::BLOCK;
					} else {
						state = State::RECOVER_AT_NEXT_BLOCK;
					}
				} else {
					throw ParsingError{tok.row, tok.col, "Expected identifier"};
				}
			} else if (key.str == "type") {
				if (tok.str == "default") {
					entry.type = TextureType::SINGLE_TEXTURE;
				} else if (tok.str == "wang") {
					entry.type = TextureType::WANG_TILES;
				} else if (tok.str == "multi4") {
					entry.type = TextureType::MULTI_x4;
				} else {
					throw ParsingError{tok.row, tok.col, "Expected texture type"};
				}
			} else if (key.str == "row") {
				if (tok.id == TOK_NUMERAL) {
					entry.y = entry.h * tok.i;
				} else {
					throw ParsingError{tok.row, tok.col, "Expected numeral"};
				}
			} else if (key.str == "col") {
				if (tok.id == TOK_NUMERAL) {
					entry.x = entry.w * tok.i;
				} else {
					throw ParsingError{tok.row, tok.col, "Expected numeral"};
				}
			} else {
				throw ParsingError{key.row, key.col, "Unknown attribute"};
			}

			state = State::BLOCK;
			continue;
		case State::RECOVER_AT_NEXT_TEXTURE:
			if (tok.id == TOK_EOF) {
				throw ParsingError{row, col, "Unexpected EOF"};
			} else if (tok.str == "texture") {
				state = State::TEX;
			}
			continue;
		case State::RECOVER_AT_NEXT_BLOCK:
			if (tok.id == TOK_EOF) {
				throw ParsingError{row, col, "Unexpected EOF"};
			} else if (tok.str == "block") {
				state = State::BLOCK;
			}
			continue;
		} // switch (state)
	} // while (true) process tokens
}

void TextureLoader::getNextChar() {
	if (ch == '\n') {
		++row;
		pos = 0;
		col = 0;
	}
	ch = getc(f);
	++pos;
	++col;
}

void TextureLoader::getNextToken() {
	enum class State {
		START,
		TERM,
		NUMERAL,
		STRING,
		ESCAPE,
		IDENT,
		COMMENT,
	};

	tok = {TOK_EOF, -1, -1, -1, ""};
	tok.str.reserve(64);
	State state = State::START;
	while (state != State::TERM) {
		switch (state) {
		case State::START:
			// eat whitespace
			while (isblank(ch) || ch == '\n')
				getNextChar();
			tok.pos = pos;
			tok.row = row;
			tok.col = col;
			// select next state
			if (ch == '\0' || ch == EOF) {
				tok.id = TOK_EOF;
				state = State::TERM;
			} else if (ch == '#') {
				state = State::COMMENT;
			} else if (ch == '{') {
				getNextChar();
				tok.id = TOK_LBRACE;
				tok.str = "{";
				state = State::TERM;
			} else if (ch == '}') {
				getNextChar();
				tok.id = TOK_RBRACE;
				tok.str = "}";
				state = State::TERM;
			} else if (ch == '=') {
				getNextChar();
				tok.id = TOK_EQUAL;
				tok.str = "=";
				state = State::TERM;
			} else if (ch == '"') {
				getNextChar();
				state = State::STRING;
			} else if (isdigit(ch)) {
				state = State::NUMERAL;
			} else if (isalpha(ch) || ch == '_') {
				state = State::IDENT;
			} else {
				throw ParsingError{row, col, "Unexpected Symbol"};
			}
			continue;
		case State::NUMERAL: {
			while (isdigit(ch)) {
				tok.str += ch;
				getNextChar();
			}
			const char *start = tok.str.c_str();
			char *end;
			tok.i = strtoul(start, &end, 10);
			if (end - start == tok.str.length()) {
				tok.id = TOK_NUMERAL;
			} else {
				throw ParsingError{row, col, "Parsing Error"};
			}

			state = State::TERM;
			continue;
		}
		case State::IDENT:
			while (isalnum(ch) || ch == '_' || ch == '.') {
				tok.str += ch;
				getNextChar();
			}
			tok.id = TOK_IDENT;
			state = State::TERM;
			continue;
		case State::STRING:
			while (ch != '"') {
				if (ch == '\\') {
					getNextChar();
					state = State::ESCAPE;
					continue;
				} else if (!isprint(ch)) {
					throw ParsingError{row, col, "Unexpected Symbol"};
				}
				tok.str += ch;
				getNextChar();
			}
			getNextChar();
			tok.id = TOK_STRING;
			state = State::TERM;
			continue;
		case State::ESCAPE:
			if (ch == 'n') {
				tok.str += '\n';
			} else if (ch == 't') {
				tok.str += '\t';
			} else if (ch == 'r') {
				tok.str += '\r';
			} else if (ch == '\\') {
				tok.str += '\\';
			} else {
				throw ParsingError{row, col, "Invalid Escape Sequence"};
			}
			getNextChar();
			state = State::STRING;
			continue;
		case State::COMMENT:
			while (ch != '\n') {
				getNextChar();
			}
			state = State::START;
			continue;
		}
	}
}

int AbstractTextureManager::load(const char *path, const BlockManager *bm) {
	auto loader = std::unique_ptr<TextureLoader>(new TextureLoader(path, bm, this));
	try {
		loader->load();
	} catch (ParsingError &e) {
		LOG(ERROR, path << ":" << (e.row + 1) << ":" << (e.col + 1) << ": " << e.error);
		return 1;
	}
	return 0;
}