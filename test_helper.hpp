/* test_helper.hpp */
#pragma once
#include <memory>
#include <string>

#include "ast.hpp"
#include "doctest.h"
#include "interpreter.hpp"
#include "parser.hpp"
#include "builtin.hpp"
// #include "declaration.hpp"

inline dara::Expr make_int(int v) { return dara::Expr{dara::IntExpr{v}, 1, 1}; }

inline dara::Expr make_str(const std::string& s) {
	return dara::Expr{dara::StringExpr{s}, 1, 1};
}

inline dara::Expr make_infix(InfixOperator op, dara::Expr lhs, dara::Expr rhs) {
	return dara::Expr{
	    dara::InfixOpExpr{op, std::make_unique<dara::Expr>(std::move(lhs)),
	                     std::make_unique<dara::Expr>(std::move(rhs))},
	    1, 1};
}

inline dara::Expr make_prefix(PrefixOperator op, dara::Expr rhs) {
	return dara::Expr{
	    dara::PrefixOpExpr{op, std::make_unique<dara::Expr>(std::move(rhs))}, 1,
	    1};
}

inline dara::Expr make_postfix(PostfixOperator op, dara::Expr lhs) {
	return dara::Expr{
	    dara::PostfixOpExpr{op, std::make_unique<dara::Expr>(std::move(lhs))}, 1,
	    1};
}

// 今回追加した Stmt/Decl 用のヘルパー
inline dara::Decl make_var_decl(const std::string& name, dara::Expr init) {
	return dara::Decl{
	    dara::VarDecl{name, std::make_unique<dara::Expr>(std::move(init))}, 1, 1};
}

inline dara::Decl make_print_decl(dara::Expr expr) {
	return dara::Decl{
	    dara::TopLevelStmt{std::make_unique<dara::Stmt>(dara::Stmt{
	        dara::PrintStmt{std::make_unique<dara::Expr>(std::move(expr))}, 1,
	        1})},
	    1, 1};
}

// test_helper.hpp (新規作成)
namespace dara::test {
dara::backend::Interpreter run_code(const std::string& code) {
	Source source(code.c_str());
	dara::frontend::Parser parser(&source);
	dara::backend::Interpreter interpreter;

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
