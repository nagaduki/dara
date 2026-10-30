#pragma once
#include <chrono>
#include <set>

#include "callable.hpp"
#include "class.hpp"
#include "error.hpp"
#include "instance.hpp"
#include "srell.hpp"
#include "value.hpp"

namespace dara::backend {

// inner class
namespace {

// =========================================================
// 1. 全角カナ (U+30A1〜U+30F4) → 半角カナ 変換テーブル
// =========================================================
struct HalfKanaPair {
	uint16_t base;  // 半角のベース文字 (0 なら対応する半角なし)
	uint16_t mark;  // 0: なし, 0xFF9E: 濁点(ﾞ), 0xFF9F: 半濁点(ﾟ)
};

// U+30A1(ァ) から U+30F4(ヴ) までの84文字を O(1) で引く定数テーブル
constexpr HalfKanaPair FULL_TO_HALF_KANA[84] = {
    {0xFF67, 0}, {0xFF71, 0},                         // ァ, ア
    {0xFF68, 0}, {0xFF72, 0},                         // ィ, イ
    {0xFF69, 0}, {0xFF73, 0},                         // ゥ, ウ
    {0xFF6A, 0}, {0xFF74, 0},                         // ェ, エ
    {0xFF6B, 0}, {0xFF75, 0},                         // ォ, オ
    {0xFF76, 0}, {0xFF76, 0xFF9E},                    // カ, ガ
    {0xFF77, 0}, {0xFF77, 0xFF9E},                    // キ, ギ
    {0xFF78, 0}, {0xFF78, 0xFF9E},                    // ク, グ
    {0xFF79, 0}, {0xFF79, 0xFF9E},                    // ケ, ゲ
    {0xFF7A, 0}, {0xFF7A, 0xFF9E},                    // コ, ゴ
    {0xFF7B, 0}, {0xFF7B, 0xFF9E},                    // サ, ザ
    {0xFF7C, 0}, {0xFF7C, 0xFF9E},                    // シ, ジ
    {0xFF7D, 0}, {0xFF7D, 0xFF9E},                    // ス, ズ
    {0xFF7E, 0}, {0xFF7E, 0xFF9E},                    // セ, ゼ
    {0xFF7F, 0}, {0xFF7F, 0xFF9E},                    // ソ, ゾ
    {0xFF80, 0}, {0xFF80, 0xFF9E},                    // タ, ダ
    {0xFF81, 0}, {0xFF81, 0xFF9E},                    // チ, ヂ
    {0xFF6F, 0}, {0xFF82, 0},      {0xFF82, 0xFF9E},  // ッ, ツ, ヅ
    {0xFF83, 0}, {0xFF83, 0xFF9E},                    // テ, デ
    {0xFF84, 0}, {0xFF84, 0xFF9E},                    // ト, ド
    {0xFF85, 0}, {0xFF86, 0},      {0xFF87, 0},
    {0xFF88, 0}, {0xFF89, 0},                         // ナ〜ノ
    {0xFF8A, 0}, {0xFF8A, 0xFF9E}, {0xFF8A, 0xFF9F},  // ハ, バ, パ
    {0xFF8B, 0}, {0xFF8B, 0xFF9E}, {0xFF8B, 0xFF9F},  // ヒ, ビ, ピ
    {0xFF8C, 0}, {0xFF8C, 0xFF9E}, {0xFF8C, 0xFF9F},  // フ, ブ, プ
    {0xFF8D, 0}, {0xFF8D, 0xFF9E}, {0xFF8D, 0xFF9F},  // ヘ, ベ, ペ
    {0xFF8E, 0}, {0xFF8E, 0xFF9E}, {0xFF8E, 0xFF9F},  // ホ, ボ, ポ
    {0xFF8F, 0}, {0xFF90, 0},      {0xFF91, 0},
    {0xFF92, 0}, {0xFF93, 0},  // マ〜モ
    {0xFF6C, 0}, {0xFF94, 0},  // ャ, ヤ
    {0xFF6D, 0}, {0xFF95, 0},  // ュ, ユ
    {0xFF6E, 0}, {0xFF96, 0},  // ョ, ヨ
    {0xFF97, 0}, {0xFF98, 0},      {0xFF99, 0},
    {0xFF9A, 0}, {0xFF9B, 0},  // ラ〜ロ
    {0, 0},      {0xFF9C, 0},      {0, 0},
    {0, 0},      {0xFF66, 0},      // ヮ, ワ, ヰ, ヱ, ヲ
    {0xFF9D, 0}, {0xFF73, 0xFF9E}  // ン, ヴ
};

// =========================================================
// 2. 半角カナ (U+FF61〜U+FF9F) → 全角カナ 変換テーブル
// =========================================================
struct FullKanaEntry {
	uint16_t normal;  // 単体の全角文字
	uint16_t
	    dakuten;  // 直後に ﾞ(0xFF9E) が続いた場合の全角文字 (0なら結合不可)
	uint16_t
	    handakuten;  // 直後に ﾟ(0xFF9F) が続いた場合の全角文字 (0なら結合不可)
};

// U+FF61(｡) から U+FF9F(ﾟ) までの63文字を O(1) で引く定数テーブル
constexpr FullKanaEntry HALF_TO_FULL_KANA[63] = {
    {0x3002, 0, 0},           {0x300C, 0, 0},
    {0x300D, 0, 0},           {0x3001, 0, 0},
    {0x30FB, 0, 0},       // ｡ ｢ ｣ ､ ･
    {0x30F2, 0x30FA, 0},  // ｦ (濁点付きで ヺ)
    {0x30A1, 0, 0},           {0x30A3, 0, 0},
    {0x30A5, 0, 0},           {0x30A7, 0, 0},
    {0x30A9, 0, 0},  // ｧ ｨ ｩ ｪ ｫ
    {0x30E3, 0, 0},           {0x30E5, 0, 0},
    {0x30E7, 0, 0},           {0x30C3, 0, 0},
    {0x30FC, 0, 0},  // ｬ ｭ ｮ ｯ ｰ
    {0x30A2, 0, 0},           {0x30A4, 0, 0},
    {0x30A6, 0x30F4, 0},      {0x30A8, 0, 0},
    {0x30AA, 0, 0},  // ｱ ｲ ｳ(ヴ) ｴ ｵ
    {0x30AB, 0x30AC, 0},      {0x30AD, 0x30AE, 0},
    {0x30AF, 0x30B0, 0},      {0x30B1, 0x30B2, 0},
    {0x30B3, 0x30B4, 0},  // ｶ〜ｺ
    {0x30B5, 0x30B6, 0},      {0x30B7, 0x30B8, 0},
    {0x30B9, 0x30BA, 0},      {0x30BB, 0x30BC, 0},
    {0x30BD, 0x30BE, 0},  // ｻ〜ｿ
    {0x30BF, 0x30C0, 0},      {0x30C1, 0x30C2, 0},
    {0x30C4, 0x30C5, 0},      {0x30C6, 0x30C7, 0},
    {0x30C8, 0x30C9, 0},  // ﾀ〜ﾄ
    {0x30CA, 0, 0},           {0x30CB, 0, 0},
    {0x30CC, 0, 0},           {0x30CD, 0, 0},
    {0x30CE, 0, 0},  // ﾅ〜ﾉ
    {0x30CF, 0x30D0, 0x30D1}, {0x30D2, 0x30D3, 0x30D4},
    {0x30D5, 0x30D6, 0x30D7}, {0x30D8, 0x30D9, 0x30DA},
    {0x30DB, 0x30DC, 0x30DD},  // ﾊ〜ﾎ
    {0x30DE, 0, 0},           {0x30DF, 0, 0},
    {0x30E0, 0, 0},           {0x30E1, 0, 0},
    {0x30E2, 0, 0},  // ﾏ〜ﾓ
    {0x30E4, 0, 0},           {0x30E6, 0, 0},
    {0x30E8, 0, 0},  // ﾔ ﾕ ﾖ
    {0x30E9, 0, 0},           {0x30EA, 0, 0},
    {0x30EB, 0, 0},           {0x30EC, 0, 0},
    {0x30ED, 0, 0},                            // ﾗ〜ﾛ
    {0x30EF, 0x30F7, 0},      {0x30F3, 0, 0},  // ﾜ(ヷ) ﾝ
    {0x309B, 0, 0},           {0x309C, 0, 0}   // ﾞ(゛) ﾟ(゜)
};

}  // namespace

// dispatcher
std::expected<Value, dara::error::InterpreterError> get_string_property(
    Interpreter& interpreter, const String& str,
    const dara::ast::Identifier& name);

// string
class BuiltinStringContains : public Callable {
   public:
	BuiltinStringContains(String str) : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_contains>";
	}

   private:
	String str;
	// bool check_inheritance(std::shared_ptr<dara::backend::Class> current,
	// std::shared_ptr<dara::backend::Class> target);
};

class BuiltinStringLength : public Callable {
   public:
	BuiltinStringLength(String str) : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_length>";
	}

   private:
	String str;
};

class BuiltinStringStartsWith : public Callable {
   public:
	BuiltinStringStartsWith(String str) : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_start_with>";
	}

   private:
	String str;
};

class BuiltinStringEndsWith : public Callable {
   public:
	BuiltinStringEndsWith(String str) : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_starts_with>";
	}

   private:
	String str;
};

// BuiltinStringSubstring in builtinn.hpp
class BuiltinStringSubstring : public Callable {
   public:
	BuiltinStringSubstring(String str) : str(std::move(str)) {}
	size_t arity() const override { return -1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_substring>";
	}

   private:
	String str;
};

// BuiltinStringSubstring in builtinn.hpp
class BuiltinStringSplit : public Callable {
   public:
	BuiltinStringSplit(String str) : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_split>";
	}

   private:
	String str;
};

// BuiltinStringSubstring in builtinn.hpp
class BuiltinStringChars : public Callable {
   public:
	BuiltinStringChars(String str) : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_chars>";
	}

   private:
	String str;
};

class BuiltinStringTrim : public Callable {
   public:
	BuiltinStringTrim(String str) : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override { return "<native fn string_trim>"; }

   private:
	String str;
};

class BuiltinStringToUpper : public Callable {
   public:
	BuiltinStringToUpper(String str) : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_toupper>";
	}

   private:
	String str;
};

class BuiltinStringToLower : public Callable {
   public:
	BuiltinStringToLower(String str) : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_tolower>";
	}

   private:
	String str;
};

class BuiltinStringToHalfwidth : public Callable {
   public:
	BuiltinStringToHalfwidth(String str) : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_to_halfwidth>";
	}

   private:
	String str;
};

class BuiltinStringToFullwidth : public Callable {
   public:
	BuiltinStringToFullwidth(String str) : str(std::move(str)) {}
	size_t arity() const override { return 0; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_to_halfwidth>";
	}

   private:
	String str;
};

class BuiltinStringReplace : public Callable {
   public:
	BuiltinStringReplace(String str) : str(std::move(str)) {}
	size_t arity() const override { return 2; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_replace>";
	}

   private:
	String str;
};

class BuiltinStringIndexOf : public Callable {
   public:
	BuiltinStringIndexOf(String str) : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_replace>";
	}

   private:
	String str;
};

class BuiltinStringReplaceRegex : public Callable {
   public:
	BuiltinStringReplaceRegex(String str) : str(std::move(str)) {}
	size_t arity() const override { return 2; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_replace_regex>";
	}

   private:
	String str;
};

class BuiltinStringMatches : public Callable {
   public:
	BuiltinStringMatches(String str) : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_replace_regex>";
	}

   private:
	String str;
};

class BuiltinStringSplitRegex : public Callable {
   public:
	BuiltinStringSplitRegex(String str) : str(std::move(str)) {}
	size_t arity() const override { return 1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_split_regex>";
	}

   private:
	String str;
};

class BuiltinStringMatchAll : public Callable {
   public:
	BuiltinStringMatchAll(String str) : str(std::move(str)) {}
	size_t arity() const override { return -1; }
	std::expected<Value, dara::error::InterpreterError> call(
	    Interpreter& interpreter, const std::vector<Value>& arguments) override;
	std::string to_string() const override {
		return "<native fn string_match_all>";
	}

   private:
	String str;
};



}  // namespace dara::backend
