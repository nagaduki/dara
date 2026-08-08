// test_interpreter.cpp
#include "doctest.h"
#include "interpreter.hpp"  // Source クラス等のために必要
#include "parser.hpp"
// #include "lexer.hpp"            // Source クラス等のために必要
// #include "combinator.hpp"
#include <memory>
#include <string>

#include "ast.hpp"  // ASTノードの定義が必要
#include "test_helper.hpp"
#include "value.hpp"

// #include <doctest/doctest.h>
// #include <iostream>
#include <memory>
// #include <string>
// #include <vnariant>

using namespace lox::frontend;
using namespace lox::backend;

TEST_CASE("Interpreter: While Statement") {
	/*
	// 複数の文を実行し、指定した変数の最終的な値を返すヘルパー関数
	auto run_script_and_get = [](const char* input, const std::string& var_name)
	-> lox::Value { Source s(input); Parser p(&s); Interpreter interpreter;

	    // 文字列の終端に到達するまで文をパース＆実行し続ける
	    while (!s.isEnd()) {
	        auto stmt_res = p.stmt();
	        if (!stmt_res) throw std::runtime_error("Parse error: " +
	stmt_res.error().message);

	        auto exec_res = interpreter.visit_stmt(stmt_res.value().get());
	//(*) if (!exec_res) throw std::runtime_error("Runtime error: " +
	exec_res.error().message); // 実行時エラー
	    }

	    //// 🌟 ここはあなたの環境(Environment)の実装に合わせて変更してください
	    //// 例: return interpreter.environment.get(var_name);
	    //return interpreter.get_environment(var_name).value();
	};
	*/
	auto run_script_and_get = [](const char* input,
	                             const std::string& var_name) -> lox::Value {
		Source s(input);
		lox::frontend::Parser p(&s);
		lox::backend::Interpreter interpreter;
		lox::Program program;

		// 1. すべてパースする (main.cpp と完全同等)
		while (!s.isEnd()) {
			spaces(&s);
			if (s.isEnd()) break;

			// stmt() ではなく decl() を使う！
			auto res = p.decl();
			if (!res)
				throw std::runtime_error("Parse error: " + res.error().message);

			program.declarations.push_back(std::move(res.value()));
		}

		// 2. すべて実行する (main.cpp と完全同等)
		for (const auto& decl : program.declarations) {
			// visit_stmt ではなく exec() を使う！
			auto exec_res = interpreter.exec(decl.get());

			// 🌟 エラーの正体を出力するようにする！
			if (!exec_res)
				throw std::runtime_error("Runtime error: " +
				                         exec_res.error().message);
		}

		// 目的の変数を抽出して返す
		//return interpreter.get_environment(var_name).value();
		return interpreter.get_variable(var_name).value();
	};

	SUBCASE("Zero iterations (Condition is initially false)") {
		const char* script =
		    "let a = 10;"
		    //"while (false) a = 20;";
		    "while(false){a=20;}";

		// 条件が最初から false なので、ループの中身は一度も実行されないはず
		CHECK(run_script_and_get(script, "a") == 10);
	}

	SUBCASE("Basic counter loop (Single statement body)") {
		const char* script =
		    "let a = 0;"
		    "while (a < 3) { a = a + 1;}";

		// 0 -> 1 -> 2 -> 3 となり、a < 3 が false になって抜ける
		CHECK(run_script_and_get(script, "a") == 3);
	}

	SUBCASE("Block statement body") {
		const char* script =
		    "let a = 0;"
		    "let b = 1;"
		    "while (a < 3) {"
		    "    a = a + 1;"
		    "    b = b * 2;"
		    "}";

		// a は 3 回インクリメントされる
		CHECK(run_script_and_get(script, "a") == 3);
		// b は 1 * 2 * 2 * 2 = 8 になるはず
		CHECK(run_script_and_get(script, "b") == 8);
	}

	SUBCASE("Complex condition with logical operators") {
		const char* script =
		    "let a = 0;"
		    "let b = 10;"
		    // a が 3未満、かつ b が 8より大きい間だけ回る (2回まわるはず)
		    "while (a < 3 and b > 8) {"
		    "    a = a + 1;"
		    "    b = b - 1;"
		    "}";

		// ループは2回で終了する (b が 8 になった時点で and の右側が false
		// になるため)
		CHECK(run_script_and_get(script, "a") == 2);
		CHECK(run_script_and_get(script, "b") == 8);
	}

	/* //
	もしネストしたスコープと環境の巻き戻しを完全に実装済みであれば、以下のテストも有効です
	SUBCASE("Nested while loops") {
	    const char* script =
	        "i = 0;"
	        "count = 0;"
	        "while (i < 3) {"
	        "    j = 0;"
	        "    while (j < 2) {"
	        "        count = count + 1;"
	        "        j = j + 1;"
	        "    }"
	        "    i = i + 1;"
	        "}";

	    // 3 * 2 = 6 回実行される
	    CHECK( run_script_and_get(script, "count") == 6 );
	}
	*/
}

TEST_CASE("Interpreter: Logical Operators (and / or)") {
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

	/*
	    auto _try_eval = [](const char* input) -> int {
	        Source s(input);
	        Parser p1(&s);
	        auto ast = p1.expr();

	        // パース失敗時はテストを落とすか例外を投げる
	        if (!ast) {
	            throw std::runtime_error("Parse error in eval: " +
	                                     ast.error().message);
	        }

	        auto ev = std::make_unique<Evaluator>();
	        return ev->eval(ast.value().get());
	    };
	*/

	auto try_eval = [](const char* input)
	    -> lox::Value {  //  変更: int ではなく lox::Value を返す
		Source s(input);
		Parser p1(&s);
		auto ast = p1.expr();
		// auto ast = p1.stmt();

		if (!ast) {
			throw std::runtime_error("Parse error in eval: " +
			                         ast.error().message);
		}

		lox::backend::Interpreter
		    interpreter;  //  変更: あなたが実装した本物のインタプリタを使う！

		// visit_expr は Result<lox::Value> (std::expected)
		// を返すはずなので受け取る
		auto res = interpreter.visit_expr(ast.value().get());
		// auto res = interpreter.visit_stmt(ast.value().get());

		if (!res) {
			// ゼロ割り等の実行時エラーが起きた場合もテストを落とす
			throw std::runtime_error("Runtime error in eval");
		}

		return res.value();  // 変更: 成功した lox::Value の中身をそのまま返す
	};

	SUBCASE("Basic Truth Tables (Booleans)") {
		CHECK(try_eval("true and true") == true);
		CHECK(try_eval("true and false") == false);
		CHECK(try_eval("false and true") == false);
		CHECK(try_eval("false and false") == false);

		CHECK(try_eval("true or true") == true);
		CHECK(try_eval("true or false") == true);
		CHECK(try_eval("false or true") == true);
		CHECK(try_eval("false or false") == false);
	}

	SUBCASE("Lox Truthiness and Return Values") {
		// Loxでは false と nil 以外はすべて true 扱い（Truthy）
		// そして、最後に評価された「実際の値」がそのまま返る仕様です

		// OR: 左がTruthyなら左を返す、Falsyなら右を返す
		CHECK(try_eval("1 or 2") == 1);
		CHECK(try_eval("false or 2") == 2);
		// CHECK( try_eval(R"("hi" or "world")") == "hi" ); //(*)

		// AND: 左がFalsyなら左を返す、Truthyなら右を返す
		CHECK(try_eval("1 and 2") == 2);
		CHECK(try_eval("false and 2") == false);
		// CHECK( try_eval(R"("hi" and "world")") == "world" ); //(*)
	}

	SUBCASE("Precedence (and has higher binding power than or)") {
		// false and true -> false, then false or true -> true
		CHECK(try_eval("false and true or true") == true);

		// true or (false and false) -> true or false -> true
		// もし優先順位が逆なら (true or false) and false -> false になるはず
		CHECK(try_eval("true or false and false") == true);
	}

	SUBCASE("Short-Circuit Evaluation (短絡評価の検証)") {
		// 短絡評価が正しく動いていれば、右辺のゼロ割りエラー(Division by
		// zero)は発生せず、 プログラムは安全に左辺の値を返すはずです。

		// OR: 左が true なので右辺 (1/0) は無視されるべき
		CHECK(try_eval("true or (1 / 0)") == true);
		CHECK(try_eval("1 or (1 / 0)") == 1);

		// AND: 左が false なので右辺 (1 / 0) は無視されるべき
		CHECK(try_eval("false and (1 / 0)") == false);
		// nil がパース・評価できる場合は以下も有効です
		// CHECK( try_eval("nil and (1 / 0)") == nil );
	}
}

TEST_CASE("Assignment Statement (AssignStmt) Tests") {
	lox::backend::Interpreter interpreter;

	SUBCASE("1. 正常系: 宣言済みの変数に新しい値を代入できる") {
		// [準備] 変数 a を宣言
		Source s1("let a = 10;");
		Parser p1(&s1);
		auto decl1 = p1.decl();
		REQUIRE(decl1.has_value());
		auto res1 = interpreter.exec(decl1.value().get());
		REQUIRE(res1.has_value());

		// [実行] 変数 a に 20 を代入
		Source s2("a = 20;");
		Parser p2(&s2);
		auto decl2 = p2.decl();
		REQUIRE(decl2.has_value());
		auto res2 = interpreter.exec(decl2.value().get());
		REQUIRE(res2.has_value());

		// [検証] Environment の値が更新されていることを確認
		// auto val = interpreter.env.get("a"); //(*)
		auto val = interpreter.get_variable_for_test("a");  //(*)
		REQUIRE(val.has_value());

		// ※ Value の比較方法は実装依存ですが、to_string などを活用します
		CHECK(lox::backend::to_string(val.value()) == "20");
	}

	SUBCASE("2. 異常系(パース時): 左辺が変数以外の場合は ParseError になる") {
		// [実行] 定数への代入を試みる
		Source s("10 = 20;");
		Parser p(&s);
		auto decl = p.decl();

		// [検証] パースの段階で確実に失敗すること（アーキテクチャの大勝利！）
		REQUIRE_FALSE(decl.has_value());
		CHECK(decl.error().message == "Invalid assignment target.");
	}

	SUBCASE(
	    "3. 異常系(実行時): 未宣言の変数への代入は InterpreterError になる") {
		// [実行] 宣言していない変数 b に代入を試みる
		Source s("b = 30;");
		Parser p(&s);
		auto decl = p.decl();

		// [検証] 文法としては正しいのでパースは成功する
		REQUIRE(decl.has_value());

		// [検証] しかし、実行時（Environment更新時）に失敗する
		auto res = interpreter.exec(decl.value().get());
		REQUIRE_FALSE(res.has_value());
		CHECK(res.error().message == "undefined variable 'b'");
	}
}

TEST_CASE("Interpreter: Environment stores strings with spaces") {
	SUBCASE("let statement and environment evaluation") {
		// 1. 変数宣言（let）のパースと実行
		Source decl_source("let message = \"hello lox\";");
		Parser p1(&decl_source);
		auto decl_res = p1.decl();
		REQUIRE(decl_res.has_value());

		lox::backend::Interpreter interpreter;  // Environment を内部に持つ

		auto exec_res = interpreter.exec(decl_res.value().get());
		REQUIRE(exec_res.has_value());  // エラーなく Environment に保存されたか

		// 2. 保存された変数の参照（VarExpr）のパースと実行
		// ※ interpreter.env は private
		// なので、直接評価して値を取り出して検証します
		Source expr_source("message");
		Parser p2(&expr_source);
		auto expr_res = p2.expr();
		REQUIRE(expr_res.has_value());

		auto eval_res = interpreter.eval(expr_res.value().get());
		REQUIRE(eval_res.has_value());

		// 3. 取り出した Value の中身が完全に一致するか検証
		lox::Value val = eval_res.value();
		REQUIRE(std::holds_alternative<std::string>(val.data));
		CHECK(std::get<std::string>(val.data) == "hello lox");
	}
}

TEST_CASE("Interpreter: Variables and Print Statement") {
	// 1. テスト用の「仮想コンソール」を用意する
	std::ostringstream capture_out;

	// 2. 仮想コンソールを繋いだインタプリタを生成
	lox::backend::Interpreter interpreter(capture_out);

	SUBCASE("Declare variables and print result") {
		// [スクリプトの内容]
		// let a = 10;
		// let b = a * 2;
		// print b;

		// ASTの構築
		lox::Decl decl1 = make_var_decl("a", make_int(10));

		/*
		Expr a_times_2 = make_infix(InfixOperator::Mul,
		                            Expr{IdentifierExpr{"a"}, 1, 1},
		                            make_int(2));
		*/
		lox::Expr a_times_2 =
		    make_infix(InfixOperator::Mul, lox::Expr{lox::VarExpr{"a"}, 1, 1},
		               make_int(2));

		lox::Decl decl2 = make_var_decl("b", std::move(a_times_2));

		// Decl print_decl = make_print_decl(Expr{IdentifierExpr{"b"}, 1, 1});
		lox::Decl print_decl =
		    make_print_decl(lox::Expr{lox::VarExpr{"b"}, 1, 1});

		// インタプリタに順次実行させる
		auto res1 = interpreter.visit_decl(&decl1);
		auto res2 = interpreter.visit_decl(&decl2);
		auto res3 = interpreter.visit_decl(&print_decl);

		// すべてエラーなく成功したかを確認
		REQUIRE(res1.has_value());
		if (!res2.has_value()) {
			INFO(" res2 error: ", res2.error().message);
		}
		REQUIRE(res2.has_value());  //(*)

		if (!res3.has_value()) {
			INFO(" res3 error: ", res3.error().message);
		}
		REQUIRE(res3.has_value());

		//  仮想コンソールに "20" と出力されているかをテスト！
		CHECK(capture_out.str() == "20\n");
	}
}

// ※ お使いのプロジェクトのヘッダーをインクルードしてください
// #include "ast.hpp"
// #include "interpreter.hpp"

// =========================================================================
// テスト用ヘルパー関数（ASTノードを簡潔に構築する）
// =========================================================================
/*
inline Expr make_int(int v) {
    return Expr{IntExpr{v}, 1, 1};
}

inline Expr make_str(const std::string& s) {
    return Expr{StringExpr{s}, 1, 1};
}

inline Expr make_infix(InfixOperator op, Expr lhs, Expr rhs) {
    return Expr{InfixOpExpr{op,
                            std::make_unique<Expr>(std::move(lhs)),
                            std::make_unique<Expr>(std::move(rhs))},
                1, 1};
}

inline Expr make_prefix(PrefixOperator op, Expr rhs) {
    return Expr{PrefixOpExpr{op, std::make_unique<Expr>(std::move(rhs))}, 1, 1};
}

inline Expr make_postfix(PostfixOperator op, Expr lhs) {
    return Expr{PostfixOpExpr{op, std::make_unique<Expr>(std::move(lhs))}, 1,
1};
}
*/
// =========================================================================
// Interpreter のテストスイート
// =========================================================================
TEST_CASE("Interpreter: Arithmetic Operations") {
	lox::backend::Interpreter interpreter;

	SUBCASE("Infix: Addition (Integer)") {
		// 10 + 20
		lox::Expr expr =
		    make_infix(InfixOperator::Add, make_int(10), make_int(20));
		auto result = interpreter.visit_expr(&expr);

		REQUIRE(result.has_value());
		// Value 構造体の中の variant (data) から int を取り出して検証
		CHECK(std::get<int>(result.value().data) == 30);
	}

	SUBCASE("Infix: Subtraction and Multiplication") {
		// 5 * 4
		lox::Expr expr_mul =
		    make_infix(InfixOperator::Mul, make_int(5), make_int(4));
		auto res_mul = interpreter.visit_expr(&expr_mul);
		REQUIRE(res_mul.has_value());
		CHECK(std::get<int>(res_mul.value().data) == 20);

		// 10 - 3
		lox::Expr expr_sub =
		    make_infix(InfixOperator::Sub, make_int(10), make_int(3));
		auto res_sub = interpreter.visit_expr(&expr_sub);
		REQUIRE(res_sub.has_value());
		CHECK(std::get<int>(res_sub.value().data) == 7);
	}

	SUBCASE("Infix: Division by Zero Error") {
		// 10 / 0
		lox::Expr expr =
		    make_infix(InfixOperator::Div, make_int(10), make_int(0));
		auto result = interpreter.visit_expr(&expr);

		// エラーが返ってくることを確認
		REQUIRE_FALSE(result.has_value());
		CHECK(result.error().message == "division by zero");
	}

	SUBCASE("Infix: String Concatenation") {
		// "Hello" + "World"
		lox::Expr expr = make_infix(InfixOperator::Add, make_str("Hello"),
		                            make_str("World"));
		auto result = interpreter.visit_expr(&expr);

		REQUIRE(result.has_value());
		CHECK(std::get<std::string>(result.value().data) == "HelloWorld");
	}

	SUBCASE("Prefix: Negation") {
		// -15
		lox::Expr expr = make_prefix(PrefixOperator::Neg, make_int(15));
		auto result = interpreter.visit_expr(&expr);

		REQUIRE(result.has_value());
		CHECK(std::get<int>(result.value().data) == -15);
	}

	SUBCASE("Postfix: Factorial") {
		// 5!
		lox::Expr expr = make_postfix(PostfixOperator::Fac, make_int(5));
		auto result = interpreter.visit_expr(&expr);

		if (!result.has_value()) {
			INFO("Error: ", result.error().message);
		}
		/*
		if(!result.has_value()) {
		    std::cerr
		        << "[DEBUG] Error Message: "
		        << result.error().message << std::endl;
		}
		*/

		REQUIRE(result.has_value());
		CHECK(std::get<int>(result.value().data) == 120);
	}

	SUBCASE("Postfix: Factorial Negative Number Error") {
		// -1!
		lox::Expr expr = make_postfix(PostfixOperator::Fac, make_int(-1));
		auto result = interpreter.visit_expr(&expr);

		REQUIRE_FALSE(result.has_value());
		CHECK(result.error().message == "factorial of negative number");
	}
}

TEST_CASE("Interpreter: ") {
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

	/*
	    auto try_eval = [](const char* input) -> int {
	        Source s(input);
	        Parser p1(&s);
	        auto ast = p1.expr();

	        // パース失敗時はテストを落とすか例外を投げる
	        if (!ast) {
	            throw std::runtime_error("Parse error in eval: " +
	                                     ast.error().message);
	        }

	        auto ev = std::make_unique<Interpreter>();
	        return ev->eval(ast.value().get());
	    };
	*/
	auto try_eval = [](const char* input)
	    -> lox::Value {  //  変更: int ではなく lox::Value を返す
		Source s(input);
		Parser p1(&s);
		auto ast = p1.expr();
		// auto ast = p1.stmt();

		if (!ast) {
			throw std::runtime_error("Parse error in eval: " +
			                         ast.error().message);
		}

		lox::backend::Interpreter
		    interpreter;  //  変更: あなたが実装した本物のインタプリタを使う！

		// visit_expr は Result<lox::Value> (std::expected)
		// を返すはずなので受け取る
		auto res = interpreter.visit_expr(ast.value().get());
		// auto res = interpreter.visit_stmt(ast.value().get());

		if (!res) {
			// ゼロ割り等の実行時エラーが起きた場合もテストを落とす
			throw std::runtime_error("Runtime error in eval");
		}

		return res.value();  // 変更: 成功した lox::Value の中身をそのまま返す
	};

	const char* case_title;
	const char* file_slug;

	// --- 既存のテスト ---
	case_title = "interpreter: base test";
	file_slug = "interpreter_base_test";
	SUBCASE(case_title) {
		CHECK(try_eval("1") == 1);
		CHECK(try_eval("42") == 42);
		// MESSAGE(file_slug, try_eval("1"));
	}

/*
	case_title = "interpreter: postfix basic";
	file_slug = "interpreter_postfix_basic";
	SUBCASE(case_title) {
		// CHECK(try_eval("1++") == 2);
		// CHECK(try_eval("42--") == 41);
		CHECK(try_eval("3!") == 6);
		// MESSAGE(file_slug, try_eval("3!"));
	}

	case_title = "interpreter: postfix with infix";
	file_slug = "interpreter_postfix_infix";
	SUBCASE(case_title) {
		CHECK_FALSE(try_eval("1+2++") == 4);    // 1 + (2++) = 1 + 3 = 4
		CHECK_FALSE(try_eval("1++*2") == 4);    // (1++) * 2 = 2 * 2 = 4
		CHECK_FALSE(try_eval("1+++2--") == 3);  // (1++) + (2--) = 2 + 1 = 3
	}

	case_title = "interpreter: postfix with prefix";
	file_slug = "interpreter_postfix_prefix";
	SUBCASE(case_title) {
		CHECK_FALSE(try_eval("-1++") == -2);    // -(1++) = -(2) = -2
		CHECK_FALSE(try_eval("-1++*2") == -4);  // (-(1++)) * 2 = (-2) * 2 = -4
	}
*/
	SUBCASE("異常系：エラーハンドリング") {
		// CHECK(try_parse("++") == "unexpected EOF");
		//CHECK(try_parse("++") == "expected an expression");
		CHECK(try_parse("++") == "++ and -- are not prefix operators");
		// 前置の'++'が実装されていなければパースは成功しつつ、評価は未定義やエラーになる場合がある
		// ここはAST構造のチェックに留める
		CHECK_FALSE(try_parse("++1") == "(++ 1)");
	}

	// --- ここから追加テスト ---

	case_title = "interpreter: prefix basic";
	file_slug = "interpreter_prefix_basic";
	SUBCASE(case_title) {
		CHECK(try_eval("-1") == -1);
		CHECK(try_eval("+2") == 2);
		// C++の論理否定の動作(!x)を想定。0なら1、それ以外なら0。
		// CHECK(try_eval("!3") == -4);
		// CHECK(try_eval("!0") == -1);
	}

	case_title = "interpreter: chained prefix";
	file_slug = "interpreter_chained_prefix";
	SUBCASE(case_title) {
		// CHECK(try_eval("!-1") == 0);   // !(-1) = 0
		CHECK(try_eval("-+1") == -1);  // -(+1) = -1
	}

	case_title = "interpreter: prefix and infix combination";
	file_slug = "interpreter_prefix_infix_combo";
	SUBCASE(case_title) {
		CHECK(try_eval("-1+2") == 1);
		CHECK(try_eval("1+-2") == -1);
		CHECK(try_eval("-1*-2") == 2);
	}

	case_title = "interpreter: strict precedence and binding power";
	file_slug = "interpreter_strict_precedence";
	SUBCASE(case_title) {
		CHECK(try_eval("-1*2") == -2);    // (-1) * 2 = -2
		CHECK(try_eval("1+-2*3") == -5);  // 1 + ((-2) * 3) = -5
		// CHECK(try_eval("!1+2") == 0);     // (!1) + 2 = 0 + 2 = 2
	}

	case_title = "interpreter: basic infix operations";
	file_slug = "interpreter_basic_infix";
	SUBCASE(case_title) {
		CHECK(try_eval("1+2") == 3);
		CHECK(try_eval("10-5") == 5);
		CHECK(try_eval("2*3") == 6);
		CHECK(try_eval("8/4") == 2);
	}

	case_title = "interpreter: infix precedence";
	file_slug = "interpreter_infix_precedence";
	SUBCASE(case_title) {
		CHECK(try_eval("1+2*3") == 7);   // 1 + (2 * 3)
		CHECK(try_eval("2*3+1") == 7);   // (2 * 3) + 1
		CHECK(try_eval("10-8/4") == 8);  // 10 - (8 / 4)
	}

	case_title = "interpreter: left associativity";
	file_slug = "interpreter_left_associativity";
	SUBCASE(case_title) {
		CHECK(try_eval("1+2+3") == 6);   // (1 + 2) + 3
		CHECK(try_eval("10-5-2") == 3);  // (10 - 5) - 2
		CHECK(try_eval("8/4/2") == 1);   // (8 / 4) / 2
	}

	case_title = "interpreter: complex expressions";
	file_slug = "interpreter_complex_expr";
	SUBCASE(case_title) {
		CHECK(try_eval("1+2*3-4/2") == 5);  // 1 + 6 - 2 = 5
		CHECK(try_eval("-1+2*3!") ==
		      11);  // -1 + 2 * (6) = 11 (※ 3!が6になる前提)
	}
}
