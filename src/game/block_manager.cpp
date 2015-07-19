#include "block_manager.hpp"

#include <vector>
#include <string>

#include "engine/logging.hpp"

enum TokenId {
	TOK_EOF,
	TOK_LBRACE,
	TOK_RBRACE,
	TOK_STRING,
	TOK_NUMERAL,
	TOK_ERROR = -1,
	TOK_NONE = -2,
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
	BlockManager *bm = nullptr;

public:
	BlockLoader(const char *path, BlockManager *bm);
	~BlockLoader();

	int load();

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

BlockLoader::BlockLoader(const char *path, BlockManager *bm) :
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

int BlockLoader::load() {
	ch = getc(f);

	std::string lastKey = "";
	std::vector<std::string> stack;
	ParserState state = FIRST;

	while (true) {
		getNextToken();

		if (tok.id == TOK_ERROR) {
			return 1;
		} else if (tok.id == TOK_EOF) {
			if (stack.size() > 0) {
				LOG(ERROR, "unexpected eof in '" << path << "'");
				return 1;
			}
			return 0;
		} else if (tok.id == TOK_STRING) {
			if (state != FIRST) {
				LOG(ERROR, "unexpected key in '" << path << "' at " << tok.row + 1 << ":" << tok.col + 1);
				return 1;
			}
			lastKey = tok.str;
			state = SECOND;
		} else if (tok.id == TOK_NUMERAL) {
			if (state != SECOND) {
				LOG(ERROR, "unexpected value in '" << path << "' at " << tok.row + 1 << ":" << tok.col + 1);
				return 1;
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
				LOG(ERROR, "unexpected { in '" << path << "' at " << tok.row + 1 << ":" << tok.col + 1);
				return 1;
			}
			stack.push_back(lastKey);
			state = FIRST;
		} else if (tok.id == TOK_RBRACE) {
			if (stack.size() <= 0) {
				LOG(ERROR, "unexpected } in '" << path << "' at " << tok.row + 1 << ":" << tok.col + 1);
				return 1;
			}
			stack.pop_back();
			state = FIRST;
		} else {
			LOG(ERROR, "unknown token in '" << path << "' at " << tok.row + 1 << ":" << tok.col + 1);
			return 1;
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
		pos = 0;
		col = 0;
	}
	ch = getc(f);
	++pos;
	++col;
}

void BlockLoader::getNextToken() {
	tok = {TOK_ERROR, -1, -1, -1, ""};
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
				tok.err = "unexpected symbol";
				tok.str = ch;
				state = TERM;
			}
			continue;
		case NUMERAL: {
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
				tok.err = "parsing error";
			}

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
	BlockManager::Entry entry{value};
	std::pair<std::string, BlockManager::Entry> pair(key, entry);
	bm->entries.insert(pair);
}

int BlockManager::load(const char *path) {
	BlockLoader *loader = nullptr;
	loader = new BlockLoader(path, this);
	auto r = loader->load();
	delete loader;
	LOG(INFO, "" << getBlockNumber() << " blocks loaded from '" << path << "'");
	return r;
}