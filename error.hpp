/* error.hpp */

#pragma once  

#include <string>
//#include <string_view>
#include <optional>
#include <sstream>

class SyntaxError {
   public:
	std::string message;
	//size_t position = 0;
	int line;
	int col;
	std::optional<char> actual;  // 文字が読めた場合は値が入り、EOFなら nullopt になる！

	std::string to_string() const {
		std::ostringstream oss;
		oss << "[line " << line << ", col " << col << "] " << message;

		// 値が存在しない（std::nullopt）なら EOF 扱い
		if (!actual.has_value()) {
			oss << " (at EOF)";
		} else {
			oss << ": '" << *actual << "'";
		}
		return oss.str();
	}
};

class InterpreterError {
   public:
	std::string message;
	//size_t position = 0;
	int line;
	int col;
	std::optional<char> actual;  // 文字が読めた場合は値が入り、EOFなら nullopt になる！
	//std::optional<char> actual; 
	std::string to_string() const {
		std::ostringstream oss;
		oss << "[line " << line << ", col " << col << "] " << message;

		// 値が存在しない（std::nullopt）なら EOF 扱い
		if (!actual.has_value()) {
			oss << " (at EOF)";
		} else {
			oss << ": '" << *actual << "'";
		}
		return oss.str();
//        return std::format("[Line {}, col {}] Interpreter Error: []", line, col, message);
	}

};


class _InterpreterError {
   public:
	std::string message;
	size_t position = 0;
	int line;
	int col;
	//std::optional<char> actual; 
	std::string to_string() const {
        return std::format("[Line {}, col {}] Interpreter Error: []", line, col, message);
        //return std::format(" Interpreter Error: []", message);

        /*
		std::ostringstream oss;
		oss << "[line " << line << ", col " << col << "] " << message;

		// 値が存在しない（std::nullopt）なら EOF 扱い
		if (!actual.has_value()) {
			oss << " (at EOF)";
		} else {
			oss << ": '" << *actual << "'";
		}
		return oss.str();
        */
	}

};

/* moden c++ error */
/*
#include <iostream>
#include <string>
#include <functional>
#include <stdexcept>

using namespace std;
using Source = const char *;

template<typename T>
using Rule = std::function<T (Source *)>;
*/
// =========================================================================
// 1. モダンな列挙型（enum class）によるエラー定義
// =========================================================================
/*
enum class _SyntaxError {
    EndOfInput,      // 入力が途中で途切れた ("too short")
    ConditionNotMet, // 条件を満たさなかった ("not satisfy")
    Unknown
};

// エラー内容を文字列に変換するヘルパー関数
std::string to_string(_SyntaxError err) {
    switch (err) {
        case SyntaxError::EndOfInput:      return "End of input reached (too short)";
        case SyntaxError::ConditionNotMet: return "Character did not satisfy condition";
        default:                          return "Unknown error";
    }
}

// =========================================================================
// 2. カスタム例外クラスの作成
// =========================================================================
class _ParseException : public std::runtime_error {
public:
    _SyntaxError error_code; // どのエラーが起きたかを enum で保持する

    // コンストラクタ
    _ParseException(_SyntaxError code) 
        : std::runtime_error(to_string(code)), error_code(code) {}
};

// 簡略化用の例外送出関数
std::runtime_error ex(_SyntaxError err) {
    return _ParseException(err);
}
*/

// =========================================================================
// パーサの実装
// =========================================================================
/*
template<typename T>
void parseTest(const Rule<T> &p, const Source &src) {
    Source s = src;
    try {
        std::cout << "Result: '" << p(&s) << "'" << std::endl;
    } 
    // ParseException なら、詳細なエラーコード（enum）を使った処理が可能
    catch ( const _ParseException &e ) {
        std::cout << "Parse Error [" << static_cast<int>(e.error_code) << "]: " << e.what() << std::endl;
    } 
    // その他の予期せぬエラー
    catch ( const std::exception &e ) {
        std::cout << "Fatal Error: " << e.what() << std::endl;
    }
}

Rule<char> satisfy(const std::function<bool (char)> &f) {
    // 【重要】[=] で変数 f をキャプチャして内部にコピーを持たせる
    Rule<char> np = [=](Source *s) {
        char ch = **s;
        if (ch == '\0') throw ex(_SyntaxError::EndOfInput);    // 文字列ではなく enum を渡す
        if (!f(ch))     throw ex(_SyntaxError::ConditionNotMet); // 文字列ではなく enum を渡す
        (*s)++;
        return ch;
    };
    return np;
}

bool isDigit(char ch) { return '0' <= ch && ch <= '9'; }

// anyChar の定義
Rule<char> anyChar = satisfy([](char) { return true; });

// digit パーサの定義（テスト用に追加）
Rule<char> digitChar = satisfy(isDigit);

int main () {
    std::cout << "--- anyChar Test ---" << std::endl;
    parseTest(anyChar, "12"); // 成功 ('1')
    parseTest(anyChar, "");   // 失敗 (EndOfInput)

    std::cout << "\n--- digitChar Test ---" << std::endl;
    parseTest(digitChar, "12"); // 成功 ('1')
    parseTest(digitChar, "a2"); // 失敗 (ConditionNotMet)
}
*/
