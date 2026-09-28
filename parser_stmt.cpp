/* parser_stmt.cpp */

#include <expected>
#include <iostream>
#include <utility>

#include "ast.hpp"
#include "combinator.hpp"
#include "error.hpp"
#include "lexer.hpp"
#include "logger.hpp"
#include "parser.hpp"
#include "source.hpp"

using namespace dara::combinator;
using namespace dara::lexer;

// 現在のファイル名と行番号を出力するマクロ
#define PRINT_LINE() \
	std::cout << "Line: " << __LINE__ << " (in " << __FILE__ << ")" << std::endl

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace dara::frontend {

/* in parser_stmt.cpp */
/*
std::expected<std::unique_ptr<dara::Stmt>, dara::error::SyntaxError>
Parser::expr_stmt( std::unique_ptr<dara::Expr> expr) { if (!Semicolon(this->s))
{
        // PRINT_LINE();
        std::string error_message = std::format(
            "Expected ';' after expression {}", this->s->peek().value());
//(****)
        // return std::unexpected(s->make_error(std::format("Expected ';' after
        // expreression");
        return std::unexpected(s->make_error(error_message));
    }
    return std::make_unique<dara::Stmt>(dara::ExprStmt{std::move(expr)});
}
*/

/* expr_stmt in parser_stmt.cpp */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::expr_stmt(std::unique_ptr<Expr> expr) {
	size_t start = expr->span.start;

	/*
	if (!Semicolon(this->s)) {
	    auto peek_res = this->s->peek();
	    std::string actual_char =
	        peek_res.has_value() ? std::string(1, peek_res.value()) : "EOF";

	    std::string error_message = std::format(
	        "Expected ';' after expression, but got '{}'", actual_char);
	    return std::unexpected(s->make_error(error_message));
	}
	*/
	if (!Semicolon(this->s)) {
		return std::unexpected(s->make_error("Expected ';' after expression"));
	}
	size_t end = s->get_current();
	return std::make_unique<Stmt>(
	    Stmt{.value = ExprStmt{std::move(expr)}, .span = Span{start, end}});
}

std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::print_stmt() {
	size_t start = s->get_current();
	// int line = this->s->line;
	// int col = this->s->col;
	// if (!LBrace(s)) {
	if (!Print(this->s)) {
		// PRINT_LINE();
		//*(this->s) = backup;
		return std::unexpected(
		    s->make_error("not 'print' at the beginning of print statement"));
	}

	auto expr_res = this->expr();

	if (!expr_res) {
		return std::unexpected(expr_res.error());
	}

	if (!Semicolon(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected ';' after value."));
	}

	size_t end = s->get_current();
	return std::make_unique<Stmt>(
	    Stmt{.value = PrintStmt{std::move(expr_res.value())},
	         .span = Span{start, end}});
}

/* assign_stmt in parser_stmt.cpp */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::assign_stmt(std::unique_ptr<Expr> lhs) {
	size_t start = lhs->span.start;

	if (!Assign(this->s)) {
		return std::unexpected(
		    s->make_error("not 'let' at the beginning of assign statement"));
	}

	auto rhs_res = this->expr();
	if (!rhs_res) {
		return std::unexpected(rhs_res.error());
	}

	if (!Semicolon(this->s)) {
		return std::unexpected(s->make_error("Expected ';' after assign."));
	}

	size_t end = s->get_current();

	return std::visit(
	    overloaded{
	        [&](VarExpr& var_expr) -> std::expected<std::unique_ptr<Stmt>,
	                                                dara::error::SyntaxError> {
		        return std::make_unique<Stmt>(Stmt{
		            .value = AssignStmt{.name = std::move(var_expr.name),
		                                .value = std::move(rhs_res.value())},
		            .span = Span{start, end}});
	        },
	        [&](GetExpr& get_expr) -> std::expected<std::unique_ptr<Stmt>,
	                                                dara::error::SyntaxError> {
		        return std::make_unique<Stmt>(
		            Stmt{.value = SetStmt{.object = std::move(get_expr.object),
		                                  .name = std::move(get_expr.name),
		                                  .value = std::move(rhs_res.value())},
		                 .span = Span{start, end}});
	        },

	        [&](auto&) -> std::expected<std::unique_ptr<Stmt>,
	                                    dara::error::SyntaxError> {
		        return std::unexpected(
		            s->make_error("Invalid assignment target.", lhs->span));
	        }},
	    lhs->value);
}

/* in parser_stmt.cpp */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::add_assign_stmt(std::unique_ptr<Expr> lhs) {
	size_t start = lhs->span.start;
	auto rhs_res = this->expr();
	if (!rhs_res) {
		return std::unexpected(rhs_res.error());
	}

	if (!Semicolon(this->s)) {
		return std::unexpected(s->make_error("Expected ';' after add assign."));
	}

	size_t end = s->get_current();

	return std::visit(
	    overloaded{
	        [&](VarExpr& var_expr) -> std::expected<std::unique_ptr<Stmt>,
	                                                dara::error::SyntaxError> {
		        return std::make_unique<Stmt>(Stmt{
		            .value =
		                CompoundAssignStmt{.name = std::move(var_expr.name),
		                                   .op = InfixOperator::Add,
		                                   .value = std::move(rhs_res.value())},
		            .span = Span{start, end}});
	        },
	        [&](GetExpr& get_expr) -> std::expected<std::unique_ptr<Stmt>,
	                                                dara::error::SyntaxError> {
		        return std::make_unique<Stmt>(Stmt{
		            .value =
		                CompoundSetStmt{.object = std::move(get_expr.object),
		                                .name = std::move(get_expr.name),
		                                .op = InfixOperator::Add,
		                                .value = std::move(rhs_res.value())},
		            .span = Span{start, end}});
	        },

	        [&](auto&) -> std::expected<std::unique_ptr<Stmt>,
	                                    dara::error::SyntaxError> {
		        return std::unexpected(
		            s->make_error("Invalid assignment target.", lhs->span));
	        }},
	    lhs->value);
}

std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::compound_assign_stmt(std::unique_ptr<Expr> lhs, InfixOperator op) {
	size_t start = lhs->span.start;

	/*
	if (!AddAssign(this->s) || !SubAssign(this->s)) {
	    return std::unexpected(s->make_error("not '++' or '--' at the beginning
	of compound statement"));
	}
	*/
	if (op == InfixOperator::Add) {
		if (!AddAssign(this->s)) {
			return std::unexpected(s->make_error(
			    "Expected '+=' at the beginning of compound statement"));
		}
	} else if (op == InfixOperator::Sub) {
		if (!SubAssign(this->s)) {
			return std::unexpected(s->make_error(
			    "Expected '-=' at the beginning of compound statement"));
		}
	}

	auto rhs_res = this->expr();
	if (!rhs_res) {
		return std::unexpected(rhs_res.error());
	}
	if (!Semicolon(this->s)) {
		return std::unexpected(
		    s->make_error("Expected ';' after compound assign"));
	}

	size_t end = s->get_current();

	return std::visit(
	    overloaded{
	        [&](VarExpr& var_expr) -> std::expected<std::unique_ptr<Stmt>,
	                                                dara::error::SyntaxError> {
		        return std::make_unique<Stmt>(Stmt{
		            .value =
		                CompoundAssignStmt{.name = std::move(var_expr.name),
		                                   .op = op,
		                                   .value = std::move(rhs_res.value())},
		            .span = Span{start, end}});
	        },
	        [&](GetExpr& get_expr) -> std::expected<std::unique_ptr<Stmt>,
	                                                dara::error::SyntaxError> {
		        return std::make_unique<Stmt>(Stmt{
		            .value =
		                CompoundSetStmt{.object = std::move(get_expr.object),
		                                .name = std::move(get_expr.name),
		                                .op = op,
		                                .value = std::move(rhs_res.value())},
		            .span = Span{start, end}});
	        },
	        [&](auto&) -> std::expected<std::unique_ptr<Stmt>,
	                                    dara::error::SyntaxError> {
		        return std::unexpected(
		            s->make_error("Invalid assignment target.", lhs->span));
	        }},
	    lhs->value);
}
/*
std::expected<std::unique_ptr<dara::Stmt>, dara::error::SyntaxError>
_parse_brace_stmt( Source* s) { if (!LBrace(s)) return
std::unexpected(s->make_error("not '{'"));

    auto inner_res = parse_stmt(s);
    if (!inner_res) return std::unexpected(inner_res.error());

    if (!RBrace(s)) return std::unexpected(s->make_error("expected '}'"));

    return std::move(inner_res.value());
}
*/

/* brace_stmt in parser_stmt.cpp */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::brace_stmt() {
	size_t start = s->get_current();
	// int line = s->line;
	// int col = s->col;

	// Source backup = *(this->s);

	if (!LBrace(s)) {
		// PRINT_LINE();
		//*(this->s) = backup;
		return std::unexpected(s->make_error("not '{'"));
	}

	std::vector<std::unique_ptr<Decl>> declarations;

	while (!s->isEnd()) {
		if (RBrace(s)) {
			break;
		}

		auto decl_res = this->decl();

		if (!decl_res) {
			return std::unexpected(decl_res.error());
		}

		declarations.push_back(std::move(decl_res.value()));
	}
	size_t end = s->get_current();

	return std::make_unique<Stmt>(
	    // Stmt{BlockStmt{std::move(declarations)}, line, col});
	    Stmt{BlockStmt{std::move(declarations)}, Span{start, end}});
}

/* else_stmt in parse_stmt.cpp */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::else_stmt() {
	// size_t start = s->get_current();
	//  int line = this->s->line;
	//  int col = this->s->col;
	auto then_res = this->brace_stmt();
	if (!then_res) {
		return std::unexpected(then_res.error());  //(***)
	}

	// s->make_error("Invalid assignment target.", lhs->span));
	return then_res;
}

/* parse_if_stmt */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::if_stmt() {
	size_t start = s->get_current();
	// size_t end = start + 1;
	//  int line = this->s->line;
	//  int col = this->s->col;

	if (!Print(this->s)) {
		// PRINT_LINE();
		//*(this->s) = backup;
		return std::unexpected(
		    s->make_error("not 'if' at the beginning of print statement"));
	}

	auto condition_res = this->expr();
	if (!condition_res) {
		return std::unexpected(condition_res.error());
	}

	auto then_res = this->brace_stmt();
	if (!then_res) {
		return std::unexpected(then_res.error());
	}

	std::unique_ptr<Stmt> else_res = nullptr;
	if (Else(this->s)) {
		auto else_block_res = this->brace_stmt();  //(*)
		if (!else_block_res) return std::unexpected(else_block_res.error());
		else_res = std::move(else_block_res.value());
	}

	size_t end = s->get_current();

	return std::make_unique<Stmt>(
	    Stmt{.value = IfStmt{std::move(condition_res.value()),
	                         std::move(then_res.value()), std::move(else_res)},
	         .span = Span{start, end}});
	//.line = line,
	//.col = col});
}

std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::while_stmt() {
	size_t start = s->get_current();

	if (!While(this->s)) {
		return std::unexpected(
		    s->make_error("not 'while' at the beginning of print statement"));
	}

	auto condition_res = this->expr();
	if (!condition_res) {
		return std::unexpected(condition_res.error());
	}

	LoopDepthGuard guard(this->loop_depth);

	auto body_res = this->brace_stmt();
	if (!body_res) {
		return std::unexpected(body_res.error());
	}

	size_t end = s->get_current();
	return std::make_unique<Stmt>(
	    Stmt{.value = WhileStmt{std::move(condition_res.value()),
	                            std::move(body_res.value())},
	         .span = Span{start, end}});
}

/* forin_stmt in parser_stmt.cpp */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::forin_stmt() {
	size_t start = s->get_current();

	if (!For(this->s)) {
		return std::unexpected(
		    s->make_error("not 'for' at the beginning of print statement"));
	}

	if (!LParen(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected '(' after 'for'."));
	}

	Let(this->s);  // 戻り値は無視して進める

	auto var_name_res = identifier(this->s);
	//
	size_t end = s->get_current();
	if (!var_name_res) {
		return std::unexpected(
		    // this->s->make_error("Expected variable name.", Span{start,
		    // end}));
		    this->s->make_error("Expected variable name."));
	}

	// std::string var_name = var_name_res.value();
	dara::ast::Identifier var_name = var_name_res.value();

	end = s->get_current();
	if (!In(this->s)) {
		return std::unexpected(this->s->make_error(
		    //"Expected 'in' after loop variable.", Span{start, end}));
		    "Expected 'in' after loop variable."));
	}

	auto iterable_res = this->expr(0);
	if (!iterable_res) {
		return std::unexpected(iterable_res.error());
	}

	end = s->get_current();
	if (!RParen(this->s)) {
		return std::unexpected(this->s->make_error(
		    //"Expected ')' after loop iterable.", Span{start, end}));
		    "Expected ')' after loop iterable."));
	}

	LoopDepthGuard guard(this->loop_depth);

	auto body_res = this->brace_stmt();
	if (!body_res) {
		return std::unexpected(body_res.error());
	}

	end = s->get_current();
	return std::make_unique<Stmt>(
	    Stmt{.value = ForInStmt{.loop_variable = var_name,
	                            .iterable = std::move(iterable_res.value()),
	                            .body = std::move(body_res.value())},
	         .span = Span{start, end}});
}

std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError> Parser::inc_stmt(
    std::unique_ptr<Expr> expr) {
	size_t start = expr->span.start;

	if (!Inc(this->s)) {
		return std::unexpected(
		    s->make_error("not '++' at the beginning of increment statement"));
	}

	if (!Semicolon(this->s)) {
		return std::unexpected(s->make_error("Expected ';' after '++'."));
	}

	// 3. 左辺(expr)が変数(VarExpr)であるかチェック
	auto var_expr = std::get_if<VarExpr>(&expr->value);
	if (!var_expr) {
		return std::unexpected(s->make_error("Invalid increment target."));
	}

	// 4. ASTノードを返す
	size_t end = s->get_current();
	return std::make_unique<Stmt>(
	    Stmt{.value = IncStmt{var_expr->name}, .span = Span{start, end}});
}

/* dec_stmt in parser_stmt.cpp */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError> Parser::dec_stmt(
    std::unique_ptr<Expr> expr) {
	size_t start = expr->span.start;

	if (!Dec(this->s)) {
		return std::unexpected(
		    s->make_error("not '--' at the beginning of decriment statement"));
	}

	if (!Semicolon(this->s)) {
		return std::unexpected(s->make_error("Expected ';' after '--'."));
	}

	// 3. 左辺(expr)が変数(VarExpr)であるかチェック
	auto var_expr = std::get_if<VarExpr>(&expr->value);
	if (!var_expr) {
		return std::unexpected(s->make_error("Invalid decrement target."));
	}

	// 4. ASTノードを返す
	size_t end = s->get_current();
	return std::make_unique<Stmt>(
	    Stmt{.value = DecStmt{var_expr->name}, .span = Span{start, end}});
}

/* return_stmt in parser_stmt.cpp */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::return_stmt() {
	size_t start = s->get_current();
	// size_t end = start + 1;
	//  PRINT_LINE();
	std::unique_ptr<Expr> return_value = nullptr;

	if (!Return(this->s)) {
		return std::unexpected(
		    s->make_error("not 'return' at the beginning of return statement"));
	}

	if (Semicolon(this->s)) {
		// PRINT_LINE();
		size_t end = s->get_current();
		// auto stmt = std::make_unique<Stmt>();
		// stmt->value = ReturnStmt{nullptr};
		// stmt->span = Span{start, end};
		// return stmt;
		return std::make_unique<Stmt>(
		    Stmt{.value = ReturnStmt{nullptr}, .span = Span{start, end}});
	}

	auto expr_res = this->expr(0);
	if (!expr_res) {
		return std::unexpected(expr_res.error());
	}

	return_value = std::move(expr_res.value());

	if (!Semicolon(this->s)) {
		return std::unexpected(
		    this->s->make_error("Expected ';' after return value"));
	}

	size_t end = s->get_current();
	// auto stmt = std::make_unique<Stmt>();  //(*)
	// stmt->value = ReturnStmt{std::move(return_value)};
	// stmt->span = Span{start, end};

	// PRINT_LINE();
	// return stmt;
	return std::make_unique<Stmt>(
	    Stmt{.value = ReturnStmt{std::move(return_value)},
	         .span = Span{start, end}});
}

/* break_stmt in parser_stmt.cpp */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::break_stmt() {
	size_t start = s->get_current();

	if (!Break(this->s)) {
		return std::unexpected(
		    s->make_error("not 'break' at the beginning of break statement"));
	}

	if (this->loop_depth == 0) {
		// PRINT_LINE();
		return std::unexpected(
		    this->s->make_error("cannot use 'break' outside of a loop"));
	}

	if (!Semicolon(this->s)) {
		return std::unexpected(
		    this->s->make_error("expected ';' after 'break'"));
	}

	size_t end = s->get_current();
	return std::make_unique<Stmt>(
	    Stmt{.value = BreakStmt{}, .span = Span{start, end}});
}

std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError>
Parser::continue_stmt() {
	size_t start = s->get_current();

	if (!Continue(this->s)) {
		return std::unexpected(s->make_error(
		    "not 'continue' at the beginning of continue statement"));
	}

	if (this->loop_depth == 0) {
		// PRINT_LINE();
		return std::unexpected(
		    this->s->make_error("cannot use 'continue' outside of a loop"));
	}

	if (!Semicolon(this->s)) {
		return std::unexpected(
		    this->s->make_error("expected ';' after 'continue'"));
	}

	size_t end = s->get_current();
	return std::make_unique<Stmt>(
	    Stmt{.value = ContinueStmt{}, .span = Span{start, end}});
}

/* stmt in parser_stmt.cpp */
std::expected<std::unique_ptr<Stmt>, dara::error::SyntaxError> Parser::stmt() {
	this->s->skip_whitespace();  //
	                             // size_t start = s->get_current();

	// auto space_res = s->skip_whitespace();
	// if (!space_res) {
	//	return std::unexpected(space_res.error());
	// }
	// Else(this->s)

	// if (auto block_res = this->brace_stmt()) {
	//	return block_res;
	// }

	Source backup = *(this->s);

	if (LBrace(this->s)) {
		*(this->s) = backup;
		return this->brace_stmt();
	}

	/*
	if (Print(this->s)) {
	    return this->print_stmt();
	}

	if (If(this->s)) {
	    return this->if_stmt();
	}

	if (While(this->s)) {
	    return this->while_stmt();
	}

	if (For(this->s)) {
	    return this->forin_stmt();
	}

	if (Return(this->s)) {
	    // PRINT_LINE();
	    return this->return_stmt();
	}

	if (Break(this->s)) {
	    return this->break_stmt();
	}

	if (Continue(this->s)) {
	    return this->continue_stmt();
	}
	*/

	if (Print(this->s)) {
		*(this->s) = backup;
		return this->print_stmt();
	}

	if (If(this->s)) {
		*(this->s) = backup;
		return this->if_stmt();
	}

	if (While(this->s)) {
		*(this->s) = backup;
		return this->while_stmt();
	}

	if (For(this->s)) {
		*(this->s) = backup;
		return this->forin_stmt();
	}

	if (Return(this->s)) {
		*(this->s) = backup;
		return this->return_stmt();
	}

	if (Continue(this->s)) {
		*(this->s) = backup;
		return this->continue_stmt();
	}

	auto expr_res = this->expr();
	if (!expr_res) {
		return std::unexpected(expr_res.error());
	}

	auto expr = std::move(expr_res.value());  //(***)

	/*
	// i++
	auto post_inc = Inc(this->s);
	if (post_inc) {
	    return this->inc_stmt(std::move(expr));
	}

	// i--
	auto post_dec = Dec(this->s);
	if (post_dec) {
	    return this->dec_stmt(std::move(expr));
	}
	*/

	if (Inc(this->s)) {
		*(this->s) = backup;
		return this->inc_stmt(std::move(expr));
	}

	if (Dec(this->s)) {
		*(this->s) = backup;
		return this->dec_stmt(std::move(expr));
	}

	/*
*   // func(, class., array[ の処理 //
	auto post_lparen = LParen(this->s);
	if (post_lparen) {
	    return this->fn_stmt(std::move(expr));
	}
	auto post_dot = Dot(this->s);
	if (post_dot) {
	    return this->dot_stmt(std::move(expr));
	}
	auto post_lbracket =LBracket(this->s) (this->s);
	if (post_lbracket) {
	    return this->array_stmt(std::move(expr));
	}
	*/

	/*
	if (Assign(this->s)) {
	    return this->assign_stmt(std::move(expr));
	}

	if (AddAssign(this->s)) {
	    return this->compound_assign_stmt(std::move(expr), InfixOperator::Add);
	}

	if (SubAssign(this->s)) {
	    return this->compound_assign_stmt(std::move(expr), InfixOperator::Sub);
	}
	*/

	if (Assign(this->s)) {
		*(this->s) = backup;
		return this->assign_stmt(std::move(expr));
	}

	if (AddAssign(this->s)) {
		*(this->s) = backup;
		return this->compound_assign_stmt(std::move(expr), InfixOperator::Add);
	}

	if (SubAssign(this->s)) {
		*(this->s) = backup;
		return this->compound_assign_stmt(std::move(expr), InfixOperator::Sub);
	}

	return this->expr_stmt(std::move(expr));  // old. TEST is OK
}

}  // namespace dara::frontend
