// #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <memory>

#include "ast.hpp"
#include "error.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "utils.hpp"
#include "builtin.hpp"

#include <algorithm>
#include <cctype>
#include <string>

// test_combinator.cpp
#include <functional>
//#include <string>

#include "combinator.hpp"
#include "interpreter.hpp"
#include "doctest.h"
#include "lexer.hpp"  // letter, digit, char1 などの基礎パーサを使うために必要


TEST_CASE("Combinator: 選択 (||) コンビネータのテスト") {
	const char* case_title;
	const char* file_slug;
    
	case_title = "基本：左側が成功する場合";
	file_slug = "base: ";
	SUBCASE(case_title) {
		auto combine = letter || digit;
		Source s("abc");
		auto p = combine;
		CHECK(p(&s) == 'a');
	}

	case_title = "";
	file_slug = "base: ";
	SUBCASE("基本：右側が成功する場合（バックトラック）") {
		auto combine = letter || digit;
		Source s("123");
		auto p = combine;
		CHECK(p(&s) == '1');
	}

	case_title = "";
	file_slug = "base: ";
	SUBCASE("複雑な選択 1") {
		auto a = char1('a');
		auto b = char1('b');
		auto c = char1('c');

		auto combine = a + b || a + c;
		Source s("ab");
		auto p = combine;
		CHECK(p(&s) == "ab");
	}

	case_title = "";
	file_slug = "base: ";
	SUBCASE("複雑な選択 2") {
		auto a = char1('a');
		auto b = char1('b');
		auto c = char1('c');

		auto combine = a + b || a + c;
		Source s("ac");
		auto p = combine;
		CHECK(p(&s) == "ac");
	}
	SUBCASE("グループ化された選択 1") {
		auto a = char1('a');
		auto b = char1('b');
		auto c = char1('c');

		auto combine = a + (b || c);
		Source s("ab");
		auto p = combine;
		CHECK(p(&s) == "ab");
	}
	SUBCASE("グループ化された選択 2") {
		auto a = char1('a');
		auto b = char1('b');
		auto c = char1('c');

		auto combine = a + (b || c);
		Source s("ac");
		auto p = combine;
		CHECK(p(&s) == std::string("ac"));
	}
}

TEST_CASE("Combinator: 連結 (+) コンビネータのテスト") {
	SUBCASE("正常系：複数パーサの連結") {
		Source s("a23");
		auto combine = letter + digit + digit;
		auto p = combine;

		auto res = p(&s);
		REQUIRE(res);
		CHECK(res.value() == "a23");
	}
	SUBCASE("異常系：途中でパースが失敗した場合") {
		Source s = "a2b";  // 3文字目が digit ではない
		auto combine = letter + digit + digit;
		auto p = combine;

		auto res = p(&s);
		REQUIRE(!res);
		CHECK(res.error().message == "not digit");

		// パース失敗時、バックトラックによりポインタが巻き戻っているか確認 ('a'
		// のまま)
		CHECK(s.peek() == 'a');
	}
}

TEST_CASE("Combinator: 繰り返し (*, many, many1) コンビネータのテスト") {
	SUBCASE("* コンビネータ：指定回数の繰り返し成功") {
		auto combine = letter * 2 + digit;
		Source s("ab3");
		auto p = combine;
		CHECK(p(&s) == "ab3");
	}
	SUBCASE("* コンビネータ：指定回数に満たない場合は失敗") {
		auto combine = letter * 2 + digit;
		Source s = "a2b";  // letter が1つしかない
		auto p = combine;
		auto res = p(&s);

		REQUIRE(!res);
		CHECK(res.error().message == "not letter");
		CHECK(s.peek() == 'a');  // 巻き戻り確認
	}
	SUBCASE("many コンビネータ：0回以上の繰り返し") {
		auto combine = many(alpha);
		Source s("abc123");
		auto p = combine;
		auto res = p(&s);

		REQUIRE(res);
		CHECK(res.value() == std::string("abc"));
	}
	SUBCASE("many コンビネータ：複数の many の連結") {
		auto combine = many(alpha) + many(digit);
		Source s("abc123");
		auto p = combine;
		auto res = p(&s);

		REQUIRE(res);
		CHECK(res.value() == std::string("abc123"));
	}
	SUBCASE("many1 コンビネータ：1回以上の繰り返し（成功）") {
		auto combine = many1(alpha);
		Source s("abc");
		auto p = combine;
		auto res = p(&s);

		REQUIRE(res);
		CHECK(res.value() == std::string("abc"));
	}
	SUBCASE("many1 コンビネータ：数字のパース") {
		auto combine = many1(digit);
		Source s("123");
		auto p = combine;
		auto res = p(&s);

		REQUIRE(res);
		CHECK(res.value() == std::string("123"));
	}
}

TEST_CASE("Combinator: 読み飛ばし (<<, >>) コンビネータのテスト") {
	SUBCASE("<< コンビネータ：右側を読み捨てて左側を返す") {
		Source s("a1");
		auto p = letter << digit;
		auto res = p(&s);

		REQUIRE(res);
		CHECK(res.value() == 'a');
	}
	SUBCASE(">> コンビネータ：左側を読み捨てて右側を返す") {
		Source s("a1");
		auto p = letter >> digit;
		auto res = p(&s);

		REQUIRE(res);
		CHECK(res.value() == '1');
	}
}

TEST_CASE("Combinator: 適用 (apply, -) コンビネータのテスト") {
	SUBCASE("apply コンビネータ：ラムダ式の適用") {
		Source s("a");
		auto p = apply<char, char>([](char ch) { return ch + 1; }, letter);
		auto res = p(&s);

		REQUIRE(res);
		CHECK(res.value() == 'b');
	}
	SUBCASE("apply コンビネータ：std::functionの適用") {
		Source s("a");
		std::function<char(const char&)> char_plus_one = [](const char& ch) {
			return ch + 1;
		};
		auto p = apply(char_plus_one, letter);
		auto res = p(&s);

		REQUIRE(res);
		CHECK(res.value() == 'b');
	}
	SUBCASE("- コンビネータ：単項マイナスの適用") {
		Source s("1");
		auto p = -integer_literal;
		auto res = p(&s);

		REQUIRE(res);
		CHECK(res.value() == -1);
	}
}
