/* parser.hpp */
#pragma once  // インクルードガードのモダンな書き方

#include <expected>
#include <memory>
#include <utility>      // 追加: std::pair のため
// #include <unordered_map>
#include "ast.hpp"
//#include "value.hpp"
// #include "combinator.hpp"
//#include "controlflow.hpp"
#include "error.hpp"
#include "lexer.hpp"
// #include "logger.hpp"

#pragma once

#include <expected>
#include <memory>

#include "ast.hpp"      // ASTノード
#include "error.hpp"    // SyntaxError等
#include "lexer.hpp"    // InfixOperator等


namespace dara::frontend {

using namespace dara::ast;

class Parser {
   private:
	Source* s;
	int function_depth = 0;
	int loop_depth = 0;

	/*
	struct FunctionDepthGuard {
	    int& depth;
	    FunctionDepthGuard(int& d) : depth(d) { depth++; }
	    ~FunctionDepthGuard() { depth--; }
	};
	*/
	struct LoopDepthGuard {
		int& depth;
		LoopDepthGuard(int& d) : depth(d) { depth++; }
		~LoopDepthGuard() { depth--; }
	};

	struct FunctionDepthGuard {
		int& function_depth;
		int& loop_depth;
		int prev_loop_depth;

		FunctionDepthGuard(int& fd, int& ld)
		    : function_depth(fd), loop_depth(ld), prev_loop_depth(ld) {
			function_depth++;
			loop_depth = 0;
		}
		~FunctionDepthGuard() {
			function_depth--;
			loop_depth = prev_loop_depth;
		}
	};

	// static const std::unordered_map<dara::lexer::InfixOperator, InfixTrait> infix_map;

   public:
	Parser(Source* s) : s(s) {};
	std::pair<int, int> infix_binding_power(dara::lexer::InfixOperator op);
	int prefix_binding_power(dara::lexer::PrefixOperator op);
	int postfix_binding_power(dara::lexer::PostfixOperator op);

	// Rule<dara::lexer::InfixOperator> infix_op();           //
	std::expected<dara::lexer::InfixOperator, dara::error::SynataxError> infix_op();  //
	// Rule<dara::lexer::PrefixOperator> prefix_op();    //(**)
	std::expected<dara::lexer::PrefixOperator, dara::error::SynataxError> prefix_op();  //
	// Rule<dara::lexer::PostfixOperator> postfix_op();  //(***)
	std::expected<dara::lexer::PostfixOperator, dara::error::SynataxError> postfix_op();  //
	                                                           //
	std::expected<dara::lexer::MixfixOperator, dara::error::SynataxError> mixfix_op();    //

	std::unique_ptr<Expr> make_atom(int value);
	std::unique_ptr<Expr> make_atom(double value);
	std::unique_ptr<Expr> make_atom(char value);
	std::unique_ptr<Expr> make_atom(std::string value);
	std::unique_ptr<Expr> make_atom(bool value);
	std::unique_ptr<Expr> make_cons(dara::lexer::InfixOperator op,
	                                     std::unique_ptr<Expr> lhs,
	                                     std::unique_ptr<Expr> rhs);
	std::unique_ptr<Expr> make_cons(dara::lexer::PrefixOperator op,
	                                     std::unique_ptr<Expr> rhs);
	std::unique_ptr<Expr> make_cons(dara::lexer::PostfixOperator op,
	                                     std::unique_ptr<Expr> lhs);
	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> apply_infix(
	    dara::lexer::InfixOperator op, std::unique_ptr<Expr> lhs,
	    std::unique_ptr<Expr> rhs, Source* s);

	/* parse series functions. */
	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> expr(int min_bp = 0);

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> prefix_expr();

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> mixfix_expr(
	    dara::lexer::MixfixOperator op, std::unique_ptr<Expr> lhs);

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> call_expr(
	    std::unique_ptr<Expr>);

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> index_expr(
	    std::unique_ptr<Expr>);

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> paren_expr();

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> bracket_expr();

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> dotdot_expr();

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> atom_expr();

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> fn_expr();

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> this_expr();

    std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> dot_expr(std::unique_ptr<Expr> left);
	//std::expected<std::unique_ptr<dara::Expr>, dara::error::SynataxError> dot_expr();

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> nud();

	std::expected<std::unique_ptr<Expr>, dara::error::SynataxError> identifier_expr();

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> stmt();

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> assign_stmt(
	    std::unique_ptr<Expr> lhs);

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> add_assign_stmt(
	    std::unique_ptr<Expr> lhs);

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> compound_assign_stmt(
	    std::unique_ptr<Expr> lhs, dara::lexer::InfixOperator op);

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> print_stmt();

	//std::expected<std::unique_ptr<dara::Stmt>, dara::error::SynataxError> assign_stmt();

	// std::expected<std::unique_ptr<dara::Stmt>, dara::error::SynataxError> expr_stmt();
	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> expr_stmt(
	    std::unique_ptr<Expr> expr);

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> brace_stmt();

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> else_stmt();

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> if_stmt();

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> inc_stmt(
	    std::unique_ptr<Expr> expr);

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> dec_stmt(
	    std::unique_ptr<Expr> expr);

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> return_stmt();

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> break_stmt();

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> continue_stmt();


	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> while_stmt();

	std::expected<std::unique_ptr<Stmt>, dara::error::SynataxError> forin_stmt();

	std::expected<std::unique_ptr<Decl>, dara::error::SynataxError> var_decl();

	std::expected<std::unique_ptr<Decl>, dara::error::SynataxError> fn_decl();

	std::expected<std::unique_ptr<Decl>, dara::error::SynataxError> class_decl();

	std::expected<std::unique_ptr<Decl>, dara::error::SynataxError> decl();

	std::expected<dara::ast::Program, dara::error::SynataxError> program();
};

}  // namespace dara::frontend
