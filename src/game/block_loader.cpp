#include "block_loader.hpp"

#include "engine/logging.hpp"

#include <cctype>
#include <memory>
#include <vector>

struct ParsingError {
	int row, col;
	std::string error;
};

enum TokenId {
	TOK_EOF,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_STRING,
	TOK_NUMERAL,
};

struct Token {
	TokenId id;
	int pos;
	int row;
	int col;
	std::string str;
	union {
		long i;
		double d;
		const char *err;
	};
};

class BlockLoader {
	const char *path;
	FILE *f = nullptr;
	AbstractBlockManager *bm = nullptr;

public:
	BlockLoader(const char *path, AbstractBlockManager *bm);
	~BlockLoader();

	void load();

private:
	int ch = 0;
	int pos = 0;
	int row = 0;
	int col = 0;
	void getNextChar();

	Token tok;
	void getNextToken();

	void emit(std::string, int);
};

BlockLoader::BlockLoader(const char *path, AbstractBlockManager *bm) :
	path(path), bm(bm)
{
	f = fopen(path, "r");
}

BlockLoader::~BlockLoader() {
	if (f) fclose(f);
}

enum ParserState {
	FIRST,
	SECOND,
};

void BlockLoader::load() {
	ch = getc(f);

	std::string lastKey = "";
	std::vector<std::string> stack;
	ParserState state = FIRST;

	while (true) {
		getNextToken();

		if (tok.id == TOK_EOF) {
			if (stack.size() > 0) {
				throw ParsingError{tok.row, tok.col, "Unexpected EOF"};
			}
			return;
		} else if (tok.id == TOK_STRING) {
			if (state != FIRST) {
				throw ParsingError{tok.row, tok.col, "Unexpected key"};
			}
			lastKey = tok.str;
			state = SECOND;
		} else if (tok.id == TOK_NUMERAL) {
			if (state != SECOND) {
				throw ParsingError{tok.row, tok.col, "Unexpected value"};
			}
			std::string key;
			for (const auto &value : stack) {
				key += value;
				key += ".";
			}
			key += lastKey;
			emit(key, tok.i);
			state = FIRST;
		} else if (tok.id == TOK_LBRACE) {
			if (lastKey == "") {
				throw ParsingError{tok.row, tok.col, "Unexpected {"};
			}
			stack.push_back(lastKey);
			state = FIRST;
		} else if (tok.id == TOK_RBRACE) {
			if (stack.size() <= 0) {
				throw ParsingError{tok.row, tok.col, "Unexpected }"};
			}
			stack.pop_back();
			state = FIRST;
		} else {
			throw ParsingError{tok.row, tok.col, "Unknown token"};
		}
	}
}

enum State {
	START,
	TERM,
	NUMERAL,
	STRING,
	COMMENT,
};

void BlockLoader::getNextChar() {
	if (ch == '\n') {
		++row;
		col = 0;
	} else {
		++pos;
		++col;
	}
	ch = getc(f);
}

void BlockLoader::getNextToken() {
	tok = {TOK_EOF, -1, -1, -1, ""};
	tok.str.reserve(64);
	State state = START;
	while (state != TERM) {
		switch (state) {
		case START:
			// eat whitespace
			while (isblank(ch) || ch == '\n')
				getNextChar();
			tok.pos = pos;
			tok.row = row;
			tok.col = col;
			// select next state
			if (ch == '\0' || ch == EOF) {
				tok.id = TOK_EOF;
				state = TERM;
			} else if (ch == '#') {
				state = COMMENT;
			} else if (ch == '{') {
				getNextChar();
				tok.id = TOK_LBRACE;
				tok.str = "{";
				state = TERM;
			} else if (ch == '}') {
				getNextChar();
				tok.id = TOK_RBRACE;
				tok.str = "}";
				state = TERM;
			} else if (isdigit(ch)) {
				state = NUMERAL;
			} else if (isalpha(ch) || ch == '_') {
				state = STRING;
			} else {
				throw ParsingError{row, col, "Unknown token"};
			}
			continue;
		case NUMERAL: {
			while (isdigit(ch)) {
				tok.str += ch;
				getNextChar();
			}
			const char *start = tok.str.c_str();
			tok.i = strtoul(start, nullptr, 10);
			tok.id = TOK_NUMERAL;

			state = TERM;
			continue;
		}
		case STRING:
			while (isalnum(ch) || ch == '_' || ch == '.') {
				tok.str += ch;
				getNextChar();
			}
			tok.id = TOK_STRING;
			state = TERM;
			continue;
		case COMMENT:
			while (ch != '\n') {
				getNextChar();
			}
			state = START;
			continue;
		}
	}
}

void BlockLoader::emit(std::string key, int value) {
	bm->add(key, value);
}

int AbstractBlockManager::load(const char *path) {
	auto loader = std::make_unique<BlockLoader>(path, this);
	try {
		loader->load();
	} catch (ParsingError &e) {
		LOG(ERROR, path << ":" << (e.row + 1) << ":" << (e.col + 1) << ": " << e.error);
		return 1;
	}
	return 0;
}
