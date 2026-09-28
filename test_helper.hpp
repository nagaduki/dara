/* test_helper.hpp */
#pragma once
#include <memory>
#include <string>

#include "ast.hpp"
#include "builtin.hpp"
#include "doctest.h"
#include "interpreter.hpp"
#include "parser.hpp"
// #include "declaration.hpp"

using namespace dara::ast;
using namespace dara::lexer;
using namespace dara::core;

inline Expr make_int(int v) { return Expr{IntExpr{v}, 1, 1}; }

inline Expr make_str(const std::string& s) { return Expr{StringExpr{s}, 1, 1}; }

inline Expr make_infix(InfixOperator op, Expr lhs, Expr rhs) {
	return Expr{InfixOpExpr{op, std::make_unique<Expr>(std::move(lhs)),
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

// 今回追加した Stmt/Decl 用のヘルパー
inline Decl make_var_decl(const std::string& name, Expr init) {
	return Decl {
		// VarDecl{name, std::make_unique<Expr>(std::move(init))}, 1, 1};
		.value = VarDecl {
			.name = Identifier{.lexeme = name, .span = Span{1, 1}},
			.initializer = std::make_unique<Expr>(std::move(init))
		}
	};
}

inline Decl make_print_decl(Expr expr) {
	return Decl{TopLevelStmt{std::make_unique<Stmt>(Stmt{
	                PrintStmt{std::make_unique<Expr>(std::move(expr))}, 1, 1})},
	            1, 1};
}

// test_helper.hpp (新規作成)
/*
namespace dara::test {
dara::backend::Interpreter run_code(const std::string& code) {
    Source source(code.c_str());
    dara::frontend::Parser parser(&source);
    //dara::backend::Interpreter interpreter = std::make_unique<Interpreter>();
    auto interpreter = std::make_unique<dara::backend::Interpreter>();

    auto program_res = parser.program();
    if (!program_res) {
        FAIL_CHECK("Parse Error");
        return interpreter;
    }

    auto eval_res = interpreter.exec(&program_res.value());
    if (!eval_res) FAIL_CHECK("Runtime Error");

    return interpreter;
}
}  // namespace dara::test
*/
