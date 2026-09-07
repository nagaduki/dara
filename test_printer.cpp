// test_printer.cpp
#include "doctest.h"
#include "printer.hpp"
#include "parser.hpp"     // parse_expr を使うために必要
#include "lexer.hpp"      // Source を使うために必要
#include "ast.hpp"        // ASTノードの定義が必要
#include "utils.hpp"      // save_to_file を使うために必要
#include "builtin.hpp"
#include <ratio>
#include <string>
#include <memory>
#include <algorithm>
#include <cctype>
#include <format>

// ============================================================================
// テスト用のヘルパー関数
// ============================================================================

// #include "parser.hpp"
// #include "interpreter.hpp"

using namespace dara::frontend;

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

// パースしてStringPrinterの出力を得るヘルパー
std::string parse_and_stringify(const char* input) {
    Source src(input);
    Parser p1(&src);
    auto ast_res = p1.expr();

    if (!ast_res) {
        return "PARSE_ERROR: " + ast_res.error().message;
    }

    // 構文木を "(A + B)" のような文字列に変換して返す
    StringPrinter printer;
    return printer.print(ast_res.value().get());
}

// ============================================================================
// 1. StringPrinter のテスト (S式フォーマットの検証)
// ============================================================================

TEST_CASE("Printer: StringPrinter generates correct S-expression format") {
    const char* case_title;
    
    case_title = "単一の数値 (IntExpr) の出力";
    SUBCASE(case_title) {
        auto expr = std::make_unique<dara::Expr>(dara::IntExpr{42});
        StringPrinter printer;
        // そのまま数値が出力されること
        CHECK(printer.print(expr.get()) == "42");
    }

    case_title = "単一の文字 (CharExpr) の出力: シングルクォート付き";
    SUBCASE(case_title) {
        auto expr = std::make_unique<dara::Expr>(dara::CharExpr{'a'});
        StringPrinter printer;
        // シングルクォートで囲まれていることの確認
        CHECK(printer.print(expr.get()) == "'a'");
    }

    case_title = "二項演算子 (InfixOpExpr) の出力: 1 + 2";
    SUBCASE(case_title) {
        auto lhs = std::make_unique<dara::Expr>(dara::IntExpr{1});
        auto rhs = std::make_unique<dara::Expr>(dara::IntExpr{2});
        auto expr = std::make_unique<dara::Expr>(
            dara::InfixOpExpr{InfixOperator::Add, std::move(lhs), std::move(rhs)});

        StringPrinter printer;
        // S式（前置記法）になっていること
        CHECK(printer.print(expr.get()) == "(+ 1 2)");
    }

    case_title = "前置演算子 (PrefixOpExpr) の出力: -5";
    SUBCASE(case_title) {
        auto rhs = std::make_unique<dara::Expr>(dara::IntExpr{5});
        auto expr = std::make_unique<dara::Expr>(
            dara::PrefixOpExpr{PrefixOperator::Neg, std::move(rhs)});

        StringPrinter printer;
        CHECK(printer.print(expr.get()) == "(- 5)");
    }

    /*
    case_title = "後置演算子 (PostfixOpExpr) の出力: 10++";
    SUBCASE(case_title) {
        auto lhs = std::make_unique<dara::Expr>(dara::IntExpr{10});
        auto expr = std::make_unique<dara::Expr>(
            dara::PostfixOpExpr{PostfixOperator::Inc, std::move(lhs)});

        StringPrinter printer;
        // S式として演算子が前に来ていること
        CHECK(printer.print(expr.get()) == "(++ 10)");
    }
    */

    case_title = "複雑なネストされた式の出力: (1 + 2) * 3";
    SUBCASE(case_title) {
        // (1 + 2) の部分
        auto lhs_inner = std::make_unique<dara::Expr>(dara::IntExpr{1});
        auto rhs_inner = std::make_unique<dara::Expr>(dara::IntExpr{2});
        auto add_expr = std::make_unique<dara::Expr>(dara::InfixOpExpr{
            InfixOperator::Add, std::move(lhs_inner), std::move(rhs_inner)});

        // * 3 の部分
        auto rhs_outer = std::make_unique<dara::Expr>(dara::IntExpr{3});
        auto expr = std::make_unique<dara::Expr>(dara::InfixOpExpr{
            InfixOperator::Mul, std::move(add_expr), std::move(rhs_outer)});

        StringPrinter printer;
        // S式のネストが正しく展開されていること
        CHECK(printer.print(expr.get()) == "(* (+ 1 2) 3)");
    }
}

// ============================================================================
// 2. DotPrinter のテスト (AST手動構築による純粋な検証)
// ============================================================================

TEST_CASE("Printer: DotPrinter generates correct DOT format (Manual AST)") {
    // SUBCASEごとに DotPrinter を新しく作るのがコツです。
    // これにより、毎回 counter が 0 にリセットされ、IDが node_00001 から始まります。
    const char* file_slug;

    SUBCASE("単一の数値 (IntExpr) の出力") {
        file_slug = "int_expr";
        // 1. テスト用のASTを手動で組み立てる (42)
        auto expr = std::make_unique<dara::Expr>(dara::IntExpr{42});

        // 2. 出力結果を取得
        DotPrinter printer;
        std::string result = printer.print(expr.get());

        // 3. 期待される文字列 (Raw文字列リテラル R"(...)" を使用)
        std::string expected = R"(digraph AST {
graph [size="8,10!", dpi=150, nodesep=0.4, ranksep=0.5];
  node [shape=box, fontname="Courier"];
node_00001 [label="integer: 42", shape=house];
}
)";
        // 4. 検証
        CHECK(result == expected);
        
        auto save_res = save_to_file(std::format("./ast_{}.dot", file_slug), result);
        if (!save_res) {
            MESSAGE("failed to save dot file.", save_res.error());
        }
    }

    SUBCASE("二項演算子 (InfixOpExpr) の出力: 10 + 20") {
        file_slug = "infixopexpr";
        // 1. テスト用のASTを手動で組み立てる (1 + 2)
        auto lhs = std::make_unique<dara::Expr>(dara::IntExpr{10});
        auto rhs = std::make_unique<dara::Expr>(dara::IntExpr{20});
        auto expr = std::make_unique<dara::Expr>(
            dara::InfixOpExpr{InfixOperator::Add, std::move(lhs), std::move(rhs)});

        // 2. 出力結果を取得
        DotPrinter printer;
        std::string result = printer.print(expr.get());

        // 3. 期待される文字列
        // 巡回順序 (Root -> Lhs -> Rhs) に従って node_ の番号が振られます
        std::string expected = R"(digraph AST {
graph [size="8,10!", dpi=150, nodesep=0.4, ranksep=0.5];
  node [shape=box, fontname="Courier"];
node_00002 [label="integer: 10", shape=house];
node_00003 [label="integer: 20", shape=house];
node_00001 [label="+"];
node_00001->node_00002;
node_00001->node_00003;
}
)";
        // 4. 検証
        CHECK(result == expected);
        
        auto save_res = save_to_file(std::format("./ast_{}.dot", file_slug), result);
        if (!save_res) {
            MESSAGE("failed to save dot file.", save_res.error());
        }
    }
}

// ============================================================================
// 3. DotPrinter とパーサの結合テスト (パース結果からのDOT生成)
// ============================================================================


TEST_CASE("Printer: DotPrinter AST to DOT format generation (Integration)") {
    const char* file_slug;

    
    SUBCASE("Infix Operator (100 + 200)") {
        file_slug = "infix_operator";
        std::string expected = R"(
digraph AST {
graph [size="8,10!", dpi=150, nodesep=0.4, ranksep=0.5];
  node [shape=box, fontname="Courier"];
node_00002 [label="integer:100", shape=house];
node_00003 [label="integer:200", shape=house];
node_00001 [label="+"];
node_00001->node_00002;
node_00001->node_00003;
}
)";
        CHECK(remove_whitespace(parse_and_dot("100 + 200")) == remove_whitespace(expected));
        
        auto save_res = save_to_file(std::format("./ast_{}.dot", file_slug), expected);
        if (!save_res) {
            MESSAGE("failed to save dot file.", save_res.error());
        }
    }


    /*SUBCASE("Prefix Operator (-1)") {
        std::string expected = R"(
digraph AST {
  node [shape=box, fontname="Courier"];
graph [size="8,10!", dpi=150, nodesep=0.4, ranksep=0.5];
node_00002 [label="integer:1", shape=house];
node_00001 [label="-"];
node_00001->node_00002;
}
)";
        CHECK(remove_whitespace(parse_and_dot("-1")) == remove_whitespace(expected));
    }*/


/*
    SUBCASE("Postfix Operator (1++)") {
        std::string expected = R"(
digraph AST {
  node [shape=box, fontname="Courier"];
node_00002 [label="integer:1", shape=house];
node_00001 [label="++"];
node_00001->node_00002;
}
)";
        CHECK(remove_whitespace(parse_and_dot("1++")) == remove_whitespace(expected));
    }
*/

/*
    SUBCASE("Complex Mixed Expression (a = -1 * 2++)") {
        file_slug = "complex_mixed";
        std::string expected = R"(
digraph AST {
  node [shape=box, fontname="Courier"];
node_00002 [label="a"];
node_00005 [label="1"];
node_00004 [label="-"];
node_00004->node_00005;
node_00007 [label="2"];
node_00006 [label="++"];
node_00006->node_00007;
node_00003 [label="*"];
node_00003->node_00004;
node_00003->node_00006;
node_00001 [label="="];
node_00001->node_00002;
node_00001->node_00003;
}
)";
        CHECK(remove_whitespace(parse_and_dot("a = -1 * 2++")) == remove_whitespace(expected));
        
        auto save_res = save_to_file(std::format("./ast_{}.dot", file_slug), expected);
        if (!save_res) {
            MESSAGE("failed to save dot file.", save_res.error());
        }
    }
*/
}
