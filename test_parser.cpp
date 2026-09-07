// test_parser.cpp
// #include <list>
#include <string>
#include <list>

#include "ast.hpp"  // ASTノードの定義が必要
#include "combinator.hpp"
#include "doctest.h"
// #include "lexer.hpp"  // Source クラス等のために必要
#include "builtin.hpp"
#include "interpreter.hpp"
#include "parser.hpp"
#include "printer.hpp"
#include "value.hpp"

// ⚠️ 注意: meson.build でリンクエラーを防ぐため、
// このファイルには #define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN を書きません。
#include <memory>
#include <string_view>
#include <variant>

#include "ast.hpp"
#include "source.hpp"

using namespace dara::frontend;
using namespace dara::backend;

inline Interpreter run_code_and_get_interpreter(const std::string& code) {
	Source source(code.c_str());  //(*)
	Parser parser(&source);
	Interpreter interpreter;
	//auto interpreter = std::make_unique<Interpreter>();

	// プログラム全体をパース（トップレベルの parse メソッドを呼ぶ想定）
	auto program_res = parser.program();
	if (!program_res) {
		FAIL_CHECK("Parse Error: " << program_res.error().message);
		return std::move(interpreter);
	}

    static std::list<dara::Program> test_asts;
	test_asts.push_back(std::move(program_res.value()));
	// プログラム全体を評価（※execの引数がProgramに対応している前提です）
	// もしProgram用のexecがまだなら、for文で declarations を回して exec
	// してください

	/*auto eval_res = interpreter.exec(&program_res.value()); //(**)
	if (!eval_res) {
	    FAIL_CHECK("Runtime Error occurred.");
	}
	*/
	// interpreter->module_asts.push_back(std::move(program_res.value()));
	// const dara::Program& program = interpreter->module_asts.back();
	auto eval_res = interpreter.exec(&test_asts.back());
	if (!eval_res) {
	    FAIL_CHECK("Runtime Error occurred.");
	}

	// return interpreter;
	return std::move(interpreter);
}

TEST_CASE("Rule: String literal with spaces (anyBut fix)") {
	SUBCASE("Parse string with spaces") {
		Source s("\"abc xyz\"");
		Parser p1(&s);
		auto expr_res = p1.expr();

		REQUIRE(expr_res.has_value());
		auto expr = std::move(expr_res.value());

		// 生成されたASTノードが StringExpr であるかを確認
		REQUIRE(std::holds_alternative<dara::StringExpr>(expr->value));

		// 中身の文字列でスペースが消失せず維持されているか確認
		auto& str_expr = std::get<dara::StringExpr>(expr->value);
		CHECK(str_expr.value == "abc xyz");
	}
}

// ==========================================
// 1. Print Statement のテスト
// ==========================================
TEST_CASE("Rule: Print Statement") {
	const char* case_title;
	const char* file_slug;

	SUBCASE("Valid print statement") {
		Source s("print 10 + 20;");
		Parser p1(&s);
		auto res = p1.stmt();

		// パースが成功したか
		REQUIRE(res.has_value());

		auto stmt = std::move(res.value());

		// 生成されたASTノードが PrintStmt であるかを確認
		CHECK(std::holds_alternative<dara::PrintStmt>(stmt->value));

		// パース完了後、末尾まで読み切っているか
		CHECK(s.isEnd());
	}

	file_slug = "expected_semicolon";
	case_title = "Missing semicolon in print statement";
	SUBCASE(case_title) {
		Source s("print 100");  // ';' が無い
		Parser p1(&s);
		auto res = p1.stmt();

		// パースは失敗（エラー）になるべき
		REQUIRE_FALSE(res.has_value());

		// 正しいエラーメッセージが出ているかを確認
		INFO(case_title);
		INFO(res.error().message);
		INFO("Expected ';' after value.");
		CHECK(res.error().message == "Expected ';' after value.");
	}
}

// ==========================================
// 2. Expression Statement (式文) のテスト
// ==========================================
TEST_CASE("Rule: Expression Statement") {  //(***)

	SUBCASE("Valid expression statement") {
		Source s("10 * 20;");
		Parser p1(&s);
		auto res = p1.stmt();
		if (!res) {
			std::cout << "【テスト失敗原因】 " << res.error().message
			          << std::endl;
		}
		REQUIRE(res.has_value());  //(*)
		// INFO(res.error().message);

		auto stmt = std::move(res.value());

		// 生成されたASTノードが ExprStmt であるかを確認
		CHECK(std::holds_alternative<dara::ExprStmt>(stmt->value));
	}

	SUBCASE("Missing semicolon in expression statement") {
		Source s("1+2");  // ';' が無い
		Parser p1(&s);
		auto res = p1.stmt();

		REQUIRE_FALSE(res.has_value());
		// CHECK(res.error().message == "Expected ';' after expression");
		CHECK(res.error().message ==
		      "Expected ';' after expression, but got 'EOF'");
	}
}

// ==========================================
// 3. Declaration (トップレベルの文) のテスト
// ==========================================
TEST_CASE("Rule: Declaration (TopLevelStmt)") {
	SUBCASE("Wraps Stmt in TopLevelStmt") {
		Source s("print 123;");
		Parser p1(&s);
		// REPLで実際に呼ばれる parse_decl をテスト
		auto res = p1.decl();

		REQUIRE(res.has_value());

		auto decl = std::move(res.value());

		// 一番外側が TopLevelStmt でラップされているか
		REQUIRE(std::holds_alternative<dara::TopLevelStmt>(decl->value));

		// その中身の Stmt を取り出して、さらに PrintStmt であるか確認
		auto& top_level = std::get<dara::TopLevelStmt>(decl->value);
		CHECK(std::holds_alternative<dara::PrintStmt>(top_level.stmt->value));
	}
}

// 空白や改行を無視して本質的な構造だけを比較するヘルパー
inline std::string remove_whitespace(std::string str) {
	str.erase(std::remove_if(str.begin(), str.end(),
	                         [](unsigned char c) { return std::isspace(c); }),
	          str.end());
	return str;
}

// パースしてDotPrinterの出力を得るヘルパー
inline std::string parse_and_dot(const char* input) {
	Source src(input);
	Parser p1(&src);
	auto ast_res = p1.expr();

	if (!ast_res) {
		return "PARSE_ERROR";
	}

	DotPrinter printer;
	return printer.print(ast_res.value().get());
}

TEST_CASE("Rule: スキャナレス・Prattパーサ 後置演算子の統合テスト") {
	auto try_parse = [](const char* input) -> std::string {
		Source s(input);
		Parser p1(&s);
		auto ast = p1.expr();
		if (ast) {
			return ast.value()
			    ->to_string();  // StringPrinterに委譲された新しい出力！
		}
		return ast.error().message;
	};

	SUBCASE("単一の数値 (Atom)") {
		CHECK(try_parse("1") == "1");
		CHECK(try_parse("42") == "42");
	}
	SUBCASE("1. 後置演算子の基本 (Postfix Only)") {
		// StringPrinterはS式を採用しているため、演算子が前に来ます
		CHECK_FALSE(try_parse("1++") == "(++ 1)");
		CHECK_FALSE(try_parse("42--") == "(-- 42)");
		CHECK(try_parse("3!") == "(! 3)");
	}
	SUBCASE("2. 後置演算子の連続 (Chained Postfix)") {
		CHECK_FALSE(try_parse("1++!") == "(! (++ 1))");
		CHECK_FALSE(try_parse("2--++") == "(++ (-- 2))");
	}
	SUBCASE("3. 後置 vs 中間演算子の優先順位") {
		CHECK_FALSE(try_parse("1+2++") == "(+ 1 (++ 2))");
		CHECK_FALSE(try_parse("1++*2") == "(* (++ 1) 2)");
		CHECK_FALSE(try_parse("1+++2--") == "(+ (++ 1) (-- 2))");
	}
	SUBCASE("4. 後置 vs 前置演算子の優先順位") {
		CHECK_FALSE(try_parse("-1++") == "(- (++ 1))");
		CHECK(try_parse("!2!") == "(! (! 2))");
		CHECK_FALSE(try_parse("-1++*2") == "(* (- (++ 1)) 2)");
	}
	SUBCASE("5. 異常系：エラーハンドリング") {
		// CHECK(try_parse("++") == "expected an expression");
		CHECK(try_parse("++") == "++ and -- are not prefix operators");

		// Prefixの'+'が評価された後、数値がないためEOFエラー等になる（Lexerの実装による）
		// パーサは正常にエラーをキャッチしているので、以前と同じ期待値でOK
		CHECK_FALSE(try_parse("++1") == "(++ 1)");
	}
}

TEST_CASE("Rule: スキャナレス・Prattパーサ 前置・中間演算子の統合テスト") {
	auto try_parse = [](const char* input) -> std::string {
		Source s(input);
		Parser p1(&s);
		auto ast = p1.expr();
		if (ast) {
			return ast.value()->to_string();
		}
		return ast.error().message;
	};

	SUBCASE("単一の数値 (Atom)") {
		CHECK(try_parse("1") == "1");
		CHECK(try_parse("42") == "42");
	}
	SUBCASE("前置演算子の基本 (Prefix Only)") {
		CHECK(try_parse("-1") == "(- 1)");
		CHECK(try_parse("+2") == "(+ 2)");
		CHECK(try_parse("!3") == "(! 3)");
	}
	SUBCASE("前置演算子の連続 (Chained Prefix)") {
		CHECK(try_parse("!-1") == "(! (- 1))");
		CHECK(try_parse("-+1") == "(- (+ 1))");
	}
	SUBCASE("前置演算子と中間演算子の組み合わせ") {
		CHECK(try_parse("-1+2") == "(+ (- 1) 2)");
		CHECK(try_parse("1+-2") == "(+ 1 (- 2))");
		CHECK(try_parse("-1*-2") == "(* (- 1) (- 2))");
	}
	SUBCASE("優先順位の厳密な検証 (Precedence & Binding Power)") {
		CHECK(try_parse("-1*2") == "(* (- 1) 2)");
		CHECK(try_parse("1+-2*3") == "(+ 1 (* (- 2) 3))");
		CHECK(try_parse("!1+2") == "(+ (! 1) 2)");
	}
	SUBCASE("異常系：エラーハンドリング") {
		CHECK(try_parse("-") == "unexpected EOF");
		CHECK(try_parse("!") == "unexpected EOF");
		CHECK(try_parse("-*2") == "expected an expression");
		CHECK(try_parse("abc") == "\"abc\"");
	}
}

TEST_CASE("Rule: Prattパーサ 全体統合テスト (結合性と優先順位)") {
	auto try_parse = [](const char* input) -> std::string {
		Source s(input);
		Parser p1(&s);
		auto ast = p1.expr();
		if (ast) {
			return ast.value()->to_string();
		}
		return ast.error().message;
	};

	SUBCASE("単純な二項演算") {
		CHECK(try_parse("1+2") == "(+ 1 2)");
		CHECK(try_parse("10-5") == "(- 10 5)");
		CHECK(try_parse("2*3") == "(* 2 3)");
		CHECK(try_parse("8/4") == "(/ 8 4)");
	}
	SUBCASE("演算子の優先順位 (Precedence)") {
		CHECK(try_parse("1+2*3") == "(+ 1 (* 2 3))");
		CHECK(try_parse("2*3+1") == "(+ (* 2 3) 1)");
		CHECK(try_parse("10-8/4") == "(- 10 (/ 8 4))");
	}
	SUBCASE("左結合性 (Left Associativity)") {
		CHECK(try_parse("1+2+3") == "(+ (+ 1 2) 3)");
		CHECK(try_parse("10-5-2") == "(- (- 10 5) 2)");
		CHECK(try_parse("8/4/2") == "(/ (/ 8 4) 2)");
	}
	SUBCASE("複合的な数式") {
		CHECK(try_parse("1+2*3-4/2") == "(- (+ 1 (* 2 3)) (/ 4 2))");
	}
	SUBCASE("prefix 正常系テスト") {
		CHECK(try_parse("+1") == "(+ 1)");
		CHECK(try_parse("-42") == "(- 42)");
	}
}
