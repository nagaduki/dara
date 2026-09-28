/* Source.hpp */
#pragma once
#include <expected>
#include <optional>
// #include <sstream>
#include <string>
#include <string_view>

#include "error.hpp"
#include "span.hpp"

namespace dara::core {

class Source {
	// const char* s;
	std::string_view data;
	size_t position = 0;

   private:
	void skip_line_comment() {
		while (true) {
			auto cur = peek();
			if (!cur || cur.value() == '\n') {
				break;
			}
			next();
		}
	}

	void skip_brock_comment() {
		next();
		next();

		while (true) {
			auto cur = peek();
			if (!cur) {
				break;
			}
			if (cur.value() == '*' && peek_offset(1) == '/') {
				next();
				next();
				break;
			}
			next();
		}
	}
	void check_fatal_nbsp() {
		//
		auto next_c = peek_offset(1);
		if (next_c && static_cast<unsigned char>(next_c.value()) == 0xA0) {
			throw dara::error::FatalSyntaxError(
			    "Invalid whitespace character (Non-width Space) "
			    "detected. Please use regular spaces",
			    Span{position, position + 2});
		}
	}

	void check_fatal_fullwidth_space() {
		auto next1 = peek_offset(1);
		auto next2 = peek_offset(2);
		if (next1 && static_cast<unsigned char>(next1.value()) == 0x80 &&
		    next2 && static_cast<unsigned char>(next2.value()) == 0x80) {
			throw dara::error::FatalSyntaxError(
			    "Invalid whitespace character (Full-width Space) "
			    "detected. Please use regular spaces",
			    Span{position, position + 3});

			//
		}
	}

   public:
	// int line;
	// int col;

	// Source(const char* s) : s(s), line(1), col(1), data(s), position(0)
	// {} Source(const char* s) : line(1), col(1), data(s), position(0) {}
	Source(const char* s) : data(s), position(0) {}

	/*
	std::expected<char, dara::error::SyntaxError> peek() {
	    if (position >= data.size())
	        return std::unexpected(make_error("too short"));
	    char c = data[position];  //(*)

	    return c;
	}
	*/
	std::expected<char, dara::error::SyntaxError> peek() {
		return peek_offset(0);
	}

	std::expected<char, dara::error::SyntaxError> peek_offset(
	    size_t offset) const {
		if (position + offset >= data.size()) {
			return std::unexpected(make_error("too short"));
		}
		return data[position + offset];
	}

	/*
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
	*/
	size_t get_current() { return this->position; }
	void next() {
		auto c = peek();  // for throw "too short"
		if (!c) {
			return;
		}
		++(this->position);
		//++(this->col);
		//++(this->position);
	}

	/* (***) */
	dara::error::SyntaxError make_error(const std::string& msg,
	                                    dara::core::Span span) const {
		// 1. 最初から「値なし（EOF）」として初期化する
		std::optional<char> actual_char = std::nullopt;

		// 2. もし安全に読める文字が残っていれば、その文字を入れる
		if (this->position < this->data.size()) {
			// actual_char = data[position];
			actual_char = data[span.start];
		}

		// auto [l, c] = this->get_line_col();
		/// auto [l, c] = this->get_line_col(this->position);

		// 3. そのまま dara::error::SyntaxError に渡す
		// return dara::error::SyntaxError{msg, line, col, actual_char};
		// return dara::error::SyntaxError{msg, this->position, l, c,
		// actual_char};
		// return dara::error::SyntaxError{msg, l, c, actual_char};
		return dara::error::
		    // SyntaxError{message : msg, span : span, actual : actual_char};
		    SyntaxError{.message = msg, .span = span, .actual = actual_char};
	}

	dara::error::SyntaxError make_error(const std::string& msg) const {
		return make_error(msg,
		                  dara::core::Span{this->position, this->position + 1});
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
	std::pair<int, int> _get_line_col() const {
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

	std::pair<int, int> get_line_col(size_t target_offset) const {
		int line = 1;
		int col = 1;
		for (size_t i = 0; i < target_offset && i < data.size(); i++) {
			char c = data[i];
			if (c == '\n') {
				line++;
				col = 1;
			} else {
				col++;
			}
		}
		return {line, col};
	};

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
			// char c = c_res.value();
			unsigned char c = static_cast<unsigned char>(c_res.value());

			// a-z0-9は問答無用で進む
			if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
				next();
			} else if (c == 0xC2) {
				check_fatal_nbsp();
				break;
			} else if (c == 0xE3) {
				check_fatal_fullwidth_space();
				break;
			} else if (c == '/') {
				auto next_c = peek_offset(1);
				if (next_c == '/') {
					skip_line_comment();
				} else if (next_c == '*') {
					skip_brock_comment();
				} else {
					break;
				}
			} else {
				break;
			}
		}
		return;
	}

	std::string get_line_string(int target_line) const {
		int current_line = 1;
		size_t line_start = 0;

		for (size_t i = 0; i <= this->data.length(); i++) {
			if (i == this->data.length() || this->data[i] == '\n') {
				if (current_line == target_line) {
					return std::string(this->data.substr(line_start, i-line_start)); 
				}
				current_line++;
				line_start = i + 1;
			}
		}
		return "";
	}
	/*
	std::expected<void, dara::error::SyntaxError> skip_whitespace() {
	    while (true) {
	        auto c_res = peek();
	        if (!c_res) {
	            return {};
	        }
	        // char c = c_res.value();
	        unsigned char c = static_cast<unsigned char>(c_res.value());

	        // a-z0-9は問答無用で進む
	        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
	            next();
	        } else if (c == 0xC2) {  //(A)
	            auto next_c = peek_offset(1);
	            if (next_c &&
	                static_cast<unsigned char>(next_c.value()) == 0xA0) {
	                return std::unexpected(make_error(
	                    "Invalid whitespace caracter (Non-Breaking Space) "
	                    "detected. Please use regular spaces"));
	            }
	            break;
	        } else if (c == 0xE3) {  //(B)
	            auto next1 = peek_offset(1);
	            auto next2 = peek_offset(2);
	            if (next1 &&
	                static_cast<unsigned char>(next1.value()) == 0x80 &&
	                next2 &&
	                static_cast<unsigned char>(next2.value()) == 0x80) {
	                return std::unexpected(make_error(
	                    "Invalid whitespace caracter (Full-width Space) "
	                    "detected. Please use regular spaces"));
	            }
	            break;
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
	    return {};
	}
	*/
};

}  // namespace dara::core
