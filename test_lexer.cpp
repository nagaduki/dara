// test_lexer.cpp
#include "doctest.h"
#include "lexer.hpp"
// 必要に応じて combinator.hpp もインクルードされている前提です（lexer.hpp内でインクルードされていればOK）
#include <string>

#include "source.hpp" // Sourceクラスの定義があるヘッダ

#include "source.hpp" 
#include "builtin.hpp"

// ==========================================
// 1. Keyword Combinator のテスト
// ==========================================
char* case_title;
char* file_slug;

using namespace dara::ast;
using namespace dara::lexer;

TEST_CASE("Lexer: Keyword 'print'") {
    SUBCASE("Valid print keyword") {
        Source s("print 1 + 2");
        auto res = Print(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == "print");
        
        // パース後、Sourceは空白を読み飛ばして '1' を指しているべき
        REQUIRE(s.peek().has_value());
        CHECK(s.peek().value() == '1');
    }

    SUBCASE("Word boundary failure (printout)") {
        Source s("printout = 10");
        auto res = Print(&s);
        // 'print' の直後にアルファベットが続くので、パース失敗(false)になるべき
        CHECK_FALSE(res.has_value()); 
    }

    SUBCASE("Word boundary failure (print123)") {
        Source s("print123");
        auto res = Print(&s);
        // 数字が続く場合も失敗すべき
        CHECK_FALSE(res.has_value());
    }
}

TEST_CASE("Lexer: Keyword 'let'") {
    SUBCASE("Valid let keyword") {
        Source s("let x = 10;");
        auto res = Let(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == "let");
        
        // パース後、Sourceは空白を読み飛ばして 'x' を指しているべき
        REQUIRE(s.peek().has_value());
        CHECK(s.peek().value() == 'x');
    }

    SUBCASE("Word boundary failure (letter)") {
        Source s("letter = 'A';");
        auto res = Let(&s);
        CHECK_FALSE(res.has_value());
    }
}

// ==========================================
// 2. Punctuation / Operator のテスト
// ==========================================

TEST_CASE("Lexer: Semicolon") {
    SUBCASE("Valid semicolon") {
        Source s(";");
        auto res = Semicolon(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == ';');
        CHECK(s.isEnd()); // 最後まで読み切ったか
    }

    SUBCASE("Semicolon with trailing spaces") {
        Source s(";   \t ");
        auto res = Semicolon(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == ';');
        // tokenコンビネータで包まれていれば、末尾の空白も消費される
        CHECK(s.isEnd()); 
    }
}

TEST_CASE("Lexer: Assign Operator") {
    SUBCASE("Valid assign") {
        Source s("= 10");
        auto res = Assign(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == '=');
    }

    SUBCASE("Fails on non-assign") {
        Source s("+ 10");
        auto res = Assign(&s);
        CHECK_FALSE(res.has_value());
    }
}

// ==========================================
// 3. 基本的なリテラルのテスト（既存の機能の保護）
// ==========================================
TEST_CASE("Lexer: Literals") {
    SUBCASE("Integer literal") {
        Source s("12345 ;");
        auto res = integer_literal(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == 12345);
        
        // パース後、Sourceは空白を飛ばして ';' を指しているべき
        REQUIRE(s.peek().has_value());
        CHECK(s.peek().value() == ';');
    }

    case_title ="Identifier Rule Test(replaced proxy)" ;
    //file_slug;
    SUBCASE(case_title) {
        Source s("my_var = 10");
        auto res = identifier(&s); // 現在識別子として代用しているもの
        REQUIRE(res.has_value());
        //INFO(case_title);
        //INFO(res.value());
        CHECK(res.value() == "my_var");
    }
}






// ==========================================
// 1. 文字列マッチと Token コンビネータのテスト
// ==========================================
TEST_CASE("Lexer: string1 and sym") {
    SUBCASE("string1 exact match") {
        Source s("print");
        auto res = string1("print")(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == "print");
    }

    SUBCASE("sym consumes trailing spaces") {
        Source s("print   123");
        auto res = sym("print")(&s); // sym = token(string1)
        REQUIRE(res.has_value());
        CHECK(s.peek().value() == '1'); // 空白が消費されていること
    }
}

// ==========================================
// 2. Keyword コンビネータのテスト（本命）
// ==========================================
TEST_CASE("Lexer: Keyword 'print'") {
    SUBCASE("Valid print keyword") {
        Source s("print 1 + 2");
        auto res = Print(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == "print");
        
        // パース後、Sourceは空白を読み飛ばして '1' を指しているべき
        REQUIRE(s.peek().has_value());
        CHECK(s.peek().value() == '1');
    }

    SUBCASE("Fails on word boundary (printout)") {
        Source s("printout = 10");
        auto res = Print(&s);
        // 'print' の直後にアルファベットが続くので、パース失敗になるべき
        CHECK_FALSE(res.has_value()); 
    }

    SUBCASE("Fails on word boundary (print123)") {
        Source s("print123");
        auto res = Print(&s);
        // 数字が続く場合も失敗すべき
        CHECK_FALSE(res.has_value());
    }
    
    // 💡 REPLでよくある罠：先頭の空白
    /*
    SUBCASE("Leading spaces trap") {
        Source s("  print 1 + 2");
        // 現在の keyword 実装は「マッチする前の」空白を読み飛ばしません。
        // そのため、REPLの入力に先頭スペースが入ると失敗します。
        // ここが false になることが、今回のバグの真犯人かもしれません。
        auto res = Print(&s);
        CHECK_FALSE(res.has_value()); 
    }
    */
}

TEST_CASE("Lexer: Keyword 'let'") {
    SUBCASE("Valid let keyword") {
        Source s("let x = 10;");
        auto res = Let(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == "let");
    }
}

// ==========================================
// 3. 記号のテスト
// ==========================================
TEST_CASE("Lexer: Semicolon") {
    SUBCASE("Valid semicolon with spaces") {
        Source s(";   \t ");
        auto res = Semicolon(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == ';');
        CHECK(s.isEnd()); // 末尾の空白まで消費されること
    }
}




// ==========================================
// 1. 文字列マッチと Token コンビネータのテスト
// ==========================================
TEST_CASE("Lexer: string1 and sym") {
    SUBCASE("string1 exact match") {
        Source s("print");
        auto res = string1("print")(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == "print");
    }

    SUBCASE("sym consumes trailing spaces") {
        Source s("print   123");
        auto res = sym("print")(&s); // sym = token(string1)
        REQUIRE(res.has_value());
        CHECK(s.peek().value() == '1'); // 空白が消費されていること
    }
}

// ==========================================
// 2. Keyword コンビネータのテスト（本命）
// ==========================================
TEST_CASE("Lexer: Keyword 'print 1'") {
    SUBCASE("Valid print keyword") {
        Source s("print 1 + 2");
        auto res = Print(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == "print");
        
        // パース後、Sourceは空白を読み飛ばして '1' を指しているべき
        REQUIRE(s.peek().has_value());
        CHECK(s.peek().value() == '1');
    }

    SUBCASE("Fails on word boundary (printout)") {
        Source s("printout = 10");
        auto res = Print(&s);
        // 'print' の直後にアルファベットが続くので、パース失敗になるべき
        CHECK_FALSE(res.has_value()); 
    }

    SUBCASE("Fails on word boundary (print123)") {
        Source s("print123");
        auto res = Print(&s);
        // 数字が続く場合も失敗すべき
        CHECK_FALSE(res.has_value());
    }
    
    // 💡 REPLでよくある罠：先頭の空白
    /*
    SUBCASE("Leading spaces trap") {
        Source s("  print 1 + 2");
        // 現在の keyword 実装は「マッチする前の」空白を読み飛ばしません。
        // そのため、REPLの入力に先頭スペースが入ると失敗します。
        // ここが false になることが、今回のバグの真犯人かもしれません。
        auto res = Print(&s);
        CHECK_FALSE(res.has_value()); 
    }
    */
}

TEST_CASE("Lexer: Keyword 'let'") {
    SUBCASE("Valid let keyword") {
        Source s("let x = 10;");
        auto res = Let(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == "let");
    }
}

// ==========================================
// 3. 記号のテスト
// ==========================================
TEST_CASE("Lexer: Semicolon") {
    SUBCASE("Valid semicolon with spaces") {
        Source s(";   \t ");
        auto res = Semicolon(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == ';');
        CHECK(s.isEnd()); // 末尾の空白まで消費されること
    }
}

TEST_CASE("Lexer: Keyword 'print' 2") {
    SUBCASE("Valid print keyword") {
        Source s("print 1 + 2");
        auto res = Print(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == "print");
        
        // パース後、Sourceは空白を読み飛ばして '1' を指しているべき
        REQUIRE(s.peek().has_value());
        CHECK(s.peek().value() == '1');
    }

    SUBCASE("Word boundary failure (printout)") {
        Source s("printout = 10");
        auto res = Print(&s);
        // 'print' の直後にアルファベットが続くので、パース失敗(false)になるべき
        CHECK_FALSE(res.has_value()); 
    }

    SUBCASE("Word boundary failure (print123)") {
        Source s("print123");
        auto res = Print(&s);
        // 数字が続く場合も失敗すべき
        CHECK_FALSE(res.has_value());
    }
}

TEST_CASE("Lexer: Keyword 'let'") {
    SUBCASE("Valid let keyword") {
        Source s("let x = 10;");
        auto res = Let(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == "let");
        
        // パース後、Sourceは空白を読み飛ばして 'x' を指しているべき
        REQUIRE(s.peek().has_value());
        CHECK(s.peek().value() == 'x');
    }

    SUBCASE("Word boundary failure (letter)") {
        Source s("letter = 'A';");
        auto res = Let(&s);
        CHECK_FALSE(res.has_value());
    }
}

// ==========================================
// 2. Punctuation / Operator のテスト
// ==========================================

TEST_CASE("Lexer: Semicolon") {
    SUBCASE("Valid semicolon") {
        Source s(";");
        auto res = Semicolon(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == ';');
        CHECK(s.isEnd()); // 最後まで読み切ったか
    }

    SUBCASE("Semicolon with trailing spaces") {
        Source s(";   \t ");
        auto res = Semicolon(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == ';');
        // tokenコンビネータで包まれていれば、末尾の空白も消費される
        CHECK(s.isEnd()); 
    }
}

TEST_CASE("Lexer: Assign Operator") {
    SUBCASE("Valid assign") {
        Source s("= 10");
        auto res = Assign(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == '=');
    }

    SUBCASE("Fails on non-assign") {
        Source s("+ 10");
        auto res = Assign(&s);
        CHECK_FALSE(res.has_value());
    }
}

// ==========================================
// 3. 基本的なリテラルのテスト（既存の機能の保護）
// ==========================================

TEST_CASE("Lexer: Literals") {
    SUBCASE("Integer literal") {
        Source s("12345 ;");
        auto res = integer_literal(&s);
        REQUIRE(res.has_value());
        CHECK(res.value() == 12345);
        
        // パース後、Sourceは空白を飛ばして ';' を指しているべき
        REQUIRE(s.peek().has_value());
        CHECK(s.peek().value() == ';');
    }

    SUBCASE("Identifier Rule Test(replaced proxy)") {
        Source s("my_var = 10");
        auto res = identifier(&s); // 現在識別子として代用しているもの
        REQUIRE(res.has_value());
        CHECK(res.value() == "my_var");
    }
}

TEST_CASE("Lexer: 単一文字のパース (char1, anyChar)") {
    SUBCASE("anyChar コンビネータの成功テスト") {
        Source s("abo");
        auto p = anyChar;
        CHECK(p(&s) == 'a');
    }

    SUBCASE("char1 正常系：指定した文字と一致する場合") {
        Source s = "abc";
        auto p = char1('a');

        // パースが成功し、'a' を返すこと
        CHECK(p(&s) == 'a');
        // パース成功後、ポインタが次の文字 ('b') に進んでいること
        CHECK(s.peek().value() == 'b');
    }

    SUBCASE("char1 異常系：入力が途切れている場合") {
        Source s = "";
        auto p = char1('a');
        auto res = p(&s);

        REQUIRE(!res);
        CHECK(res.error().message == "not char a");
        // '\0' との比較から、値がないこと（EOF）の確認
        CHECK(!res.error().actual.has_value());
        CHECK(res.error().to_string() == "[line 1, col 1] not char a (at EOF)");
    }

    SUBCASE("char1 応用系：連続してパースできるか") {
        Source s = "ab";

        auto p_a = char1('a');
        auto res_a = p_a(&s);
        auto p_b = char1('b');
        auto res_b = p_b(&s);

        CHECK(res_a == 'a');  // 1文字目の 'a' を消費
        CHECK(res_b == 'b');  // 2文字目の 'b' を消費
        CHECK(s.isEnd() == true); // 最後まで到達したか確認
    }
}

TEST_CASE("Lexer: 文字種判定のパース (digit, upper, lower, alpha)") {
    SUBCASE("digit 正常系：指定した文字が数字") {
        Source s = "123";
        auto p = digit;
        auto res = p(&s);

        CHECK(res == '1');
        CHECK(s.peek() == '2');
    }

    SUBCASE("digit 異常系：指定した文字が数字ではない") {
        Source s = "xyz";
        auto p = digit;
        
        auto res = p(&s);
        CHECK(!res); 
        CHECK(res.error().message == std::string("not digit"));
        // パース失敗時、ポインタが進んでいないこと ('x' のまま)
        CHECK(s.peek().value() == 'x');
    }

    SUBCASE("upper 正常系：指定した文字が大文字") {
        Source s = "ABC";
        auto p = upper;
        auto res = p(&s);

        CHECK(res == 'A');
        CHECK(s.peek() == 'B');
    }

    SUBCASE("upper 異常系：指定した文字が大文字ではない") {
        Source s = "xyz";
        auto p = upper;
        auto res = p(&s);

        CHECK(!res);
        CHECK(res.error().message == "not upper");
        CHECK(s.peek() == 'x');
    }

    SUBCASE("lower 正常系：指定した文字が小文字") {
        Source s = "abc";
        auto p = lower;
        auto res = p(&s);

        CHECK(res == 'a');
        CHECK(s.peek() == 'b');
    }

    SUBCASE("lower 異常系：指定した文字が小文字ではない") {
        Source s = "XYZ";
        auto p = lower;
        auto res = p(&s);

        CHECK(!res);
        CHECK(res.error().message == "not lower");
        CHECK(s.peek() == 'X');
    }

    SUBCASE("alpha 正常系：指定した文字がアルファベット") {
        Source s = "abc";
        auto p = alpha;
        auto res = p(&s);

        CHECK(res == 'a');
        CHECK(s.peek() == 'b');
    }

    SUBCASE("alpha 異常系：指定した文字がアルファベットではない") {
        Source s = "123";
        auto p = alpha;
        auto res = p(&s);

        CHECK(!res);
        CHECK(res.error().message == "not alpha");
        CHECK(s.peek() == '1');
    }
}

TEST_CASE("Lexer: 文字列・数値リテラルのパース (string1, integer_literal)") {
    SUBCASE("string1 コンビネータの成功テスト (単一)") {
        Source s("abc");
        auto res = string1("abc")(&s);

        CHECK(res);
        CHECK(res.value() == "abc");
    }

    SUBCASE("string1 コンビネータの成功テスト (選択肢)") {
        auto combine = string1("ab") || string1("ac");
        Source s("ac");
        auto p = combine;
        auto res = p(&s);
        
        CHECK(res);
        CHECK(res.value() == std::string("ac"));
    }

    SUBCASE("integer_literal コンビネータの成功テスト") {
        Source s("12345");
        auto p = integer_literal;
        auto res = p(&s);
        
        CHECK(res);
        CHECK(res.value() == 12345);
    }

    SUBCASE("integer_literal コンビネータの失敗テスト (オーバーフロー)") {
        Source s("9999999999999999999999999999999999999");
        auto p = integer_literal;
        auto res = p(&s);
        
        CHECK(!res);
        CHECK(res.error().message == "number too large");
    }
}
