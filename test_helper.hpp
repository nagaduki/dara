#pragma once
#include <memory>
#include <string>

#include "ast.hpp"
#include "doctest.h"
#include "interpreter.hpp"
#include "parser.hpp"
// #include "declaration.hpp"

inline lox::Expr make_int(int v) { return lox::Expr{lox::IntExpr{v}, 1, 1}; }

inline lox::Expr make_str(const std::string& s) {
	return lox::Expr{lox::StringExpr{s}, 1, 1};
}

inline lox::Expr make_infix(InfixOperator op, lox::Expr lhs, lox::Expr rhs) {
	return lox::Expr{
	    lox::InfixOpExpr{op, std::make_unique<lox::Expr>(std::move(lhs)),
	                     std::make_unique<lox::Expr>(std::move(rhs))},
	    1, 1};
}

inline lox::Expr make_prefix(PrefixOperator op, lox::Expr rhs) {
	return lox::Expr{
	    lox::PrefixOpExpr{op, std::make_unique<lox::Expr>(std::move(rhs))}, 1,
	    1};
}

inline lox::Expr make_postfix(PostfixOperator op, lox::Expr lhs) {
	return lox::Expr{
	    lox::PostfixOpExpr{op, std::make_unique<lox::Expr>(std::move(lhs))}, 1,
	    1};
}

// 今回追加した Stmt/Decl 用のヘルパー
inline lox::Decl make_var_decl(const std::string& name, lox::Expr init) {
	return lox::Decl{
	    lox::VarDecl{name, std::make_unique<lox::Expr>(std::move(init))}, 1, 1};
}

inline lox::Decl make_print_decl(lox::Expr expr) {
	return lox::Decl{
	    lox::TopLevelStmt{std::make_unique<lox::Stmt>(lox::Stmt{
	        lox::PrintStmt{std::make_unique<lox::Expr>(std::move(expr))}, 1,
	        1})},
	    1, 1};
}

// test_helper.hpp (新規作成)
namespace lox::test {
lox::backend::Interpreter run_code(const std::string& code) {
	Source source(code.c_str());
	lox::frontend::Parser parser(&source);
	lox::backend::Interpreter interpreter;

	auto program_res = parser.program();
	if (!program_res) {
		FAIL_CHECK("Parse Error");
		return interpreter;
	}

	auto eval_res = interpreter.exec(&program_res.value());
	if (!eval_res) FAIL_CHECK("Runtime Error");

	return interpreter;
}
}  // namespace lox::test
