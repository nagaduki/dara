/* Source.hpp */
#pragma once
#include <expected>
#include <optional>
// #include <sstream>
#include <string>
#include <string_view>

#include "error.hpp"

class Source {
	// const char* s;
	std::string_view data;
	size_t position = 0;

   public:
	int line;
	int col;

	// Source(const char* s) : s(s), line(1), col(1), data(s), position(0) {}
	// Source(const char* s) : line(1), col(1), data(s), position(0) {}
	Source(const char* s) : data(s), position(0) {}

	std::expected<char, SyntaxError> peek() {
		if (position >= data.size())
			return std::unexpected(make_error("too short"));
		char c = data[position];  //(*)

		return c;
	}

	void next() {
		auto c = peek();  // for throw "too short"
		if (!c) {
			return;
		}
		if (c.value() == '\n') {
			++(this->line);
			this->col = 0;
		}
		//++(this->s);:w
		// j f
		++(this->col);
		++(this->position);
	}

	void _next() {
		auto c = peek();  // for throw "too short"
		if (!c) {
			return;
		}
		++(this->position);
		//++(this->col);
		//++(this->position);
	}

	SyntaxError make_error(const std::string& msg) const {
		// 1. 最初から「値なし（EOF）」として初期化する
		std::optional<char> actual_char = std::nullopt;

		// 2. もし安全に読める文字が残っていれば、その文字を入れる
		if (position < data.size()) {
			actual_char = data[position];
		}

		auto [l, c] = this->get_line_col();

		// 3. そのまま SyntaxError に渡す
		// return SyntaxError{msg, line, col, actual_char};
		// return SyntaxError{msg, this->position, l, c, actual_char};
		return SyntaxError{msg, l, c, actual_char};
	}

	bool isEnd() const {
		// return *s == '\0';
		// return this->position >= this->data.size();
		return this->position >= this->data.size();
	}
	// bool operator==(const Source& src) const { return s == src.s; }
	bool operator==(const Source& src) const { return this->data == src.data; }
	/* 上のoverloadされた==を使って!=をoverload */
	bool operator!=(const Source& src) const { return !(*this == src); }

	// エラー発生時にだけ呼ぶヘルパー
	std::pair<int, int> get_line_col() const {
		int line = 1;
		int col = 1;
		for (size_t i = 0; i < position; i++) {
			char c = data[i];

			if (c == '\r') {
				if (i + 1 < data.size() && data[i + 1] == '\n') {
					i++;
				}
				line++;
				col = 1;
			} else if (c == '\n') {
				line++;
				col = 1;
			} else {
				col++;
			}
		}
		return {line, col};
	}

	std::optional<char> peek_next() const {
		if (position + 1 >= data.size()) {
			return std::nullopt;
		}
		return data[position + 1];
	}

    // comment out
	void skip_whitespace() {
		while (true) {
			auto c_res = peek();
			if (!c_res) {
				return;
			}
			char c = c_res.value();

            // a-z0-9は問答無用で進む
			if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
				next();
			} else if (c == '/') {
				auto next_c = peek_next();

                // route
				if (next_c == '/') {
					while (true) {
						auto cur = peek();
						if (!cur || cur.value() == '\n') {
							break;
						}
						next();
					}
                // /* .. */ route
				} else if (next_c == '*') {
					next();
					next();

					while (true) {
						auto cur = peek();
						if (!cur) {
							break;
						}

						if (cur.value() == '*' && peek_next() == '/') {
							next();
							next();
							break;
						}
						next();
					}
				} else {
					break;
				}
			} else {
				break;
			}
		}
	}
};
