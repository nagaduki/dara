/* parser.hpp */
#pragma once  // インクルードガードのモダンな書き方

#include <expected>
#include <memory>
// #include <unordered_map>
#include "ast.hpp"
#include "value.hpp"
// #include "combinator.hpp"
#include "controlflow.hpp"
#include "error.hpp"
#include "lexer.hpp"
// #include "logger.hpp"

namespace lox::frontend {

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

	// static const std::unordered_map<InfixOperator, InfixTrait> infix_map;

   public:
	Parser(Source* s) : s(s) {};
	std::pair<int, int> infix_binding_power(InfixOperator op);
	int prefix_binding_power(PrefixOperator op);
	int postfix_binding_power(PostfixOperator op);

	// Rule<InfixOperator> infix_op();           //
	std::expected<InfixOperator, SyntaxError> infix_op();  //
	// Rule<PrefixOperator> prefix_op();    //(**)
	std::expected<PrefixOperator, SyntaxError> prefix_op();  //
	// Rule<PostfixOperator> postfix_op();  //(***)
	std::expected<PostfixOperator, SyntaxError> postfix_op();  //
	                                                           //
	std::expected<MixfixOperator, SyntaxError> mixfix_op();    //

	std::unique_ptr<lox::Expr> make_atom(int value);
	std::unique_ptr<lox::Expr> make_atom(char value);
	std::unique_ptr<lox::Expr> make_atom(std::string value);
	std::unique_ptr<lox::Expr> make_atom(bool value);
	std::unique_ptr<lox::Expr> make_cons(InfixOperator op,
	                                     std::unique_ptr<lox::Expr> lhs,
	                                     std::unique_ptr<lox::Expr> rhs);
	std::unique_ptr<lox::Expr> make_cons(PrefixOperator op,
	                                     std::unique_ptr<lox::Expr> rhs);
	std::unique_ptr<lox::Expr> make_cons(PostfixOperator op,
	                                     std::unique_ptr<lox::Expr> lhs);
	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> apply_infix(
	    InfixOperator op, std::unique_ptr<lox::Expr> lhs,
	    std::unique_ptr<lox::Expr> rhs, Source* s);

	/* parse series functions. */
	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> expr(int min_bp = 0);

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> prefix_expr();

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> mixfix_expr(
	    MixfixOperator op, std::unique_ptr<lox::Expr> lhs);

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> call_expr(
	    std::unique_ptr<lox::Expr>);

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> index_expr(
	    std::unique_ptr<lox::Expr>);

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> paren_expr();

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> bracket_expr();

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> dotdot_expr();

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> atom_expr();

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> fn_expr();

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> this_expr();

    std::expected<std::unique_ptr<lox::Expr>, SyntaxError> dot_expr(std::unique_ptr<lox::Expr> left);
	//std::expected<std::unique_ptr<lox::Expr>, SyntaxError> dot_expr();

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> nud();

	std::expected<std::unique_ptr<lox::Expr>, SyntaxError> identifier_expr();

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> stmt();

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> assign_stmt(
	    std::unique_ptr<lox::Expr> lhs);

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> add_assign_stmt(
	    std::unique_ptr<lox::Expr> lhs);

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> compound_assign_stmt(
	    std::unique_ptr<lox::Expr> lhs, InfixOperator op);

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> print_stmt();

	//std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> assign_stmt();

	// std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> expr_stmt();
	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> expr_stmt(
	    std::unique_ptr<lox::Expr> expr);

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> brace_stmt();

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> else_stmt();

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> if_stmt();

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> inc_stmt(
	    std::unique_ptr<lox::Expr> expr);

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> dec_stmt(
	    std::unique_ptr<lox::Expr> expr);

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> return_stmt();

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> break_stmt();

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> continue_stmt();


	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> while_stmt();

	std::expected<std::unique_ptr<lox::Stmt>, SyntaxError> forin_stmt();

	std::expected<std::unique_ptr<lox::Decl>, SyntaxError> var_decl();

	std::expected<std::unique_ptr<lox::Decl>, SyntaxError> fn_decl();

	std::expected<std::unique_ptr<lox::Decl>, SyntaxError> class_decl();

	std::expected<std::unique_ptr<lox::Decl>, SyntaxError> decl();

	std::expected<lox::Program, SyntaxError> program();
};

}  // namespace lox::frontend
